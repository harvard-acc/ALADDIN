/* Implementation of an accelerator datapath that uses a private scratchpad for
 * local memory.
 */

#include <functional>  // For std::hash
#include <string>
#include <sstream>
#include <vector>

#include "aladdin/common/DatabaseDeps.h"

#include "base/statistics.hh"
#include "aladdin/common/cacti-p/cacti_interface.h"
#include "aladdin/common/cacti-p/io.h"
#include "aladdin/common/ExecNode.h"
#include "debug/CacheDatapath.hh"
#include "CacheDatapath.h"

CacheDatapath::CacheDatapath(const Params *p) :
    BaseDatapath(p->benchName, p->traceFileName, p->configFileName),
    Gem5Datapath(p, p->acceleratorId, p->executeStandalone, p->system),
    load_queue(p->loadQueueSize,
               p->loadBandwidth,
               p->acceleratorName + ".load_queue",
               p->loadQueueCacheConfig),
    store_queue(p->storeQueueSize,
                p->storeBandwidth,
                p->acceleratorName + ".store_queue",
                p->storeQueueCacheConfig),
    _dataMasterId(p->system->getMasterId(name() + ".data")),
    dcachePort(this),
    tickEvent(this),
    retryPkt(NULL),
    isCacheBlocked(false),
    cacheSize(p->cacheSize),
    cacheLineSize(p->cacheLineSize),
    cacheHitLatency(p->cacheHitLatency),
    cacheAssoc(p->cacheAssoc),
    cacti_cfg(p->cactiCacheConfig),
    accelerator_name(p->acceleratorName),
    dtb(this,
        p->tlbEntries,
        p->tlbAssoc,
        p->tlbHitLatency,
        p->tlbMissLatency,
        p->tlbPageBytes,
        p->isPerfectTLB,
        p->numOutStandingWalks,
        p->tlbBandwidth,
        p->tlbCactiConfig,
        p->acceleratorName),
    inFlightNodes(0)
{
  BaseDatapath::use_db = p->useDb;
  BaseDatapath::experiment_name = p->experimentName;
  BaseDatapath::cycleTime = p->cycleTime;
  initAddress();
  setGlobalGraph();
  globalOptimizationPass();
  setGraphForStepping();
  registerStats();
  num_cycles = 0;
  tokenizeString(p->acceleratorDeps, accelerator_deps);
  system->registerAccelerator(accelerator_id, this, accelerator_deps);
  if (execute_standalone)
    scheduleOnEventQueue(1);
}

CacheDatapath::~CacheDatapath() {}

BaseMasterPort &
CacheDatapath::getMasterPort(const string &if_name, PortID idx)
{
    // Get the right port based on name. This applies to all the
    // subclasses of the base CPU and relies on their implementation
    // of getDataPort and getInstPort. In all cases there methods
    // return a MasterPort pointer.
    if (if_name == "dcache_port")
        return getDataPort();
    else
        return MemObject::getMasterPort(if_name);
}

//dcachePort interface
bool
CacheDatapath::DcachePort::recvTimingResp(PacketPtr pkt)
{
  DPRINTF(CacheDatapath, "recvTimingResp for address: %#x %s\n",
          pkt->getAddr(), pkt->cmdString());
  if (pkt->isError())
    DPRINTF(CacheDatapath, "Got error packet back for address: %#x\n", pkt->getAddr());
  DatapathSenderState *state = dynamic_cast<DatapathSenderState*>(pkt->senderState);
  if (!state->is_ctrl_signal)
  {
    datapath->completeDataAccess(pkt);
  }
  else
  {
    DPRINTF(CacheDatapath, "recvTimingResp for control signal access: %#x\n",
            pkt->getAddr());
    delete state;
    delete pkt->req;
    delete pkt;
  }
  return true;
}

void
CacheDatapath::DcachePort::recvTimingSnoopReq(PacketPtr pkt)
{
  DPRINTF(CacheDatapath, "recvTimingSnoopReq for addr:%#x %s\n", pkt->getAddr(), pkt->cmdString());
  if (pkt->isInvalidate())
  {
    DPRINTF(CacheDatapath, "received invalidation for addr:%#x\n", pkt->getAddr());
  }
}

void
CacheDatapath::DcachePort::recvRetry()
{
  DPRINTF(CacheDatapath, "recvRetry for addr:");
  assert(datapath->isCacheBlocked && datapath->retryPkt != NULL);

  DatapathSenderState *state = dynamic_cast<DatapathSenderState*>(
      datapath->retryPkt->senderState);
  if (!state->is_ctrl_signal)
  {
    /* A retry constitutes a read of the load/store queues. This code must be
     * executed first, before the retryPkt is nullified. However, this only
     * applies if the access was not a control signal access.
     */
    if (datapath->retryPkt->cmd == MemCmd::ReadReq)
      datapath->load_queue.readStats ++;
    else if (datapath->retryPkt->cmd == MemCmd::WriteReq)
      datapath->store_queue.readStats ++;
  }

  DPRINTF(CacheDatapath, "%#x \n", datapath->retryPkt->getAddr());
  if (datapath->dcachePort.sendTimingReq(datapath->retryPkt))
  {
    DPRINTF(CacheDatapath, "Retry pass!\n");
    datapath->retryPkt = NULL;
    datapath->isCacheBlocked = false;
  }
  else
    DPRINTF(CacheDatapath, "Still blocked!\n");
}

void
CacheDatapath::completeDataAccess(PacketPtr pkt)
{
  DatapathSenderState *state = dynamic_cast<DatapathSenderState *> (pkt->senderState);
  unsigned node_id = state->node_id;
  DPRINTF(CacheDatapath, "completeDataAccess for node_id %d, addr:%#x %s\n",
          node_id, pkt->getAddr(), pkt->cmdString());
  /* Loop through the executingQueue to update the status of memory
   * operations, mark nodes accessing data on the same cache line ready to fire.
   * Remember that these are physical addresses.
   */
  Addr blockAddr = pkt->getAddr() / cacheLineSize;
  if (inflight_mem_ops.find(node_id) != inflight_mem_ops.end())
  {
    for (auto it = executingQueue.begin(); it != executingQueue.end(); ++it)
    {
      ExecNode* node = *it;
      unsigned n_id = node->get_node_id();
      if (node->is_memory_op() && inflight_mem_ops[n_id].status == WaitingFromCache)
      {
        Addr addr = inflight_mem_ops[n_id].paddr;
        if (addr / cacheLineSize == blockAddr)
          inflight_mem_ops[n_id].status = Returned;
      }
    }
    assert(inflight_mem_ops[node_id].status == Returned);
  }
  DPRINTF(CacheDatapath, "node:%d mem access is returned\n", node_id);
  /* Data that is returned gets written back into the load store queues. */
  if (pkt->cmd == MemCmd::ReadReq)
    load_queue.writeStats ++;
  else if (pkt->cmd == MemCmd::WriteReq)
    store_queue.writeStats ++;
  delete state;
  delete pkt->req;
  delete pkt;
}

void
CacheDatapath::finishTranslation(PacketPtr pkt, bool was_miss)
{
  DPRINTF(CacheDatapath, "finishTranslation for addr:%#x %s\n",
          pkt->getAddr(), pkt->cmdString());
  AladdinTLB::TLBSenderState *state =
      dynamic_cast<AladdinTLB::TLBSenderState*>(pkt->senderState);
  unsigned node_id = state->node_id;
  assert(inflight_mem_ops.find(node_id) != inflight_mem_ops.end());
  assert(inflight_mem_ops[node_id].status == Translating);
  /* If the TLB missed, we need to retry the complete instruction, as the dcache
   * state may have changed during the waiting time, so we go back to Ready
   * instead of Translated. The next translation request should hit.
   */
  if (was_miss) {
    inflight_mem_ops[node_id].status = Ready;
  } else {
    // Translations are actually the complete memory address, not just the page
    // number, because of the trace to virtual address translation.
    Addr paddr = *pkt->getPtr<Addr>();
    inflight_mem_ops[node_id].status = Translated;
    inflight_mem_ops[node_id].paddr = paddr;
  }
  DPRINTF(CacheDatapath, "node:%d mem access is translated\n", node_id);
  delete state;
  delete pkt->req;
  delete pkt;
}

bool
CacheDatapath::accessTLB(Addr addr, unsigned size, bool isLoad, int node_id)
{
  DPRINTF(CacheDatapath, "accessTLB for trace addr:%#x\n", addr);
  //form request
  Request *req = NULL;
  //physical request
  Flags<FlagsType> flags = 0;
  //constructor for physical request only
  req = new Request (addr, size/8, flags, dataMasterId());

  MemCmd command;
  if (isLoad)
    command = MemCmd::ReadReq;
  else
    command = MemCmd::WriteReq;
  PacketPtr data_pkt = new Packet(req, command);

  Addr* translation = new Addr;
  *translation = 0x0;  // Signifies no translation.
  data_pkt->dataStatic<Addr>(translation);

  AladdinTLB::TLBSenderState *state = new AladdinTLB::TLBSenderState(node_id);
  data_pkt->senderState = state;

  //TLB access
  if (dtb.translateTiming(data_pkt))
    return true;
  else
  {
    data_pkt->deleteData();
    delete state;
    delete data_pkt->req;
    delete data_pkt;
    return false;
  }
}

bool
CacheDatapath::accessCache(
    Addr addr, unsigned size, bool isLoad, int node_id, long long int value)
{
  DPRINTF(CacheDatapath, "accessCache for addr:%#x\n", addr);
  bool queues_available = (isLoad && load_queue.can_issue()) ||
                          (!isLoad && store_queue.can_issue());
  if (!queues_available)
  {
    DPRINTF(CacheDatapath, "Load/store queues are full or no more ports are available.\n");
    return false;
  }
  if (isCacheBlocked)
  {
    DPRINTF(CacheDatapath, "MSHR is full. Waiting for misses to return.\n");
    return false;
  }

  // Bandwidth limitations.
  if (isLoad) {
    load_queue.issued_this_cycle ++;
    load_queue.writeStats ++;
  }
  else {
    store_queue.issued_this_cycle ++;
    store_queue.writeStats ++;
  }

  Request *req = NULL;
  Flags<FlagsType> flags = 0;
  /* To use strided prefetching, we need to include a "PC" so the prefetcher
   * can predict memory behavior similar to how branch predictors work. We
   * don't have real PCs in aladdin so we'll just hash the unique id of the
   * node.  */
  std::string unique_id = exec_nodes[node_id]->get_static_node_id();
  Addr pc = static_cast<Addr>(std::hash<std::string>()(unique_id));
  req = new Request( addr, size/8, flags, dataMasterId(), curTick(), pc);
  /* The context id and thread ids are needed to pass a few assert checks in
   * gem5, but they aren't actually required for the mechanics of the memory
   * checking itsef. This has to be set outside of the constructor or the
   * address will be interpreted as a virtual, not physical. */
  req->setThreadContext(context_id, thread_id);

  MemCmd command;
  long long int *data = new long long int;
  *data = value;
  if (isLoad) {
    command = MemCmd::ReadReq;
  } else {
    command = MemCmd::WriteReq;
  }
  PacketPtr data_pkt = new Packet(req, command);
  data_pkt->dataStatic<long long int>(data);

  DatapathSenderState *state = new DatapathSenderState(node_id);
  data_pkt->senderState = state;

  if(!dcachePort.sendTimingReq(data_pkt))
  {
    //blocked, retry
    assert(retryPkt == NULL);
    retryPkt = data_pkt;
    isCacheBlocked = true;
    DPRINTF(CacheDatapath, "port is blocked. Will retry later...\n");
  }
  else
  {
    if (isLoad)
      DPRINTF(CacheDatapath, "Node id %d load from address %#x issued to "
              "dcache!\n", node_id, addr);
    else
      DPRINTF(CacheDatapath, "Node id %d store of value %d to address %#x "
              "issued to dcache!\n", node_id, value, addr);
  }

  return true;
}

void CacheDatapath::sendFinishedSignal()
{
  Flags<FlagsType> flags = 0;
  MemCmd command = MemCmd::WriteReq;
  size_t size = 4;
  Request *req = new Request(finish_flag, size, flags, dataMasterId());
  req->setThreadContext(context_id, thread_id);
  PacketPtr data_pkt = new Packet(req, command);
  DatapathSenderState *state = new DatapathSenderState(-1, true);
  data_pkt->senderState = state;
  uint8_t *data = new uint8_t[size];
  // Write a sentinel value 4 bytes long.
  for (int i = 0; i < size; i++)
    data[i] = i;
  data_pkt->dataStatic<uint8_t>(data);

  if(!dcachePort.sendTimingReq(data_pkt))
  {
    assert(retryPkt == NULL);
    retryPkt = data_pkt;
    DPRINTF(CacheDatapath, "Failed to send control signal, retrying...\n");
  }
  else
  {
    DPRINTF(CacheDatapath, "Sent control signal to %#x.\n", finish_flag);
  }
}

void CacheDatapath::globalOptimizationPass()
{
  // Node removals must come first.
  removeInductionDependence();
  removePhiNodes();
  // Base address must be initialized next.
  initBaseAddress();
  loopFlatten();
  loopUnrolling();
  removeSharedLoads();
  storeBuffer();
  removeRepeatedStores();
  treeHeightReduction();
  // Must do loop pipelining last; after all the data/control dependences are fixed
  loopPipelining();
}

void CacheDatapath::copyToExecutingQueue()
{
  auto it = readyToExecuteQueue.begin();
  while (it != readyToExecuteQueue.end()) {
    ExecNode* node = *it;
    if (node->is_store_op())
      executingQueue.push_front(node);
    else
      executingQueue.push_back(node);
    it = readyToExecuteQueue.erase(it);
  }
}

void CacheDatapath::resetCacheCounters()
{
  load_queue.issued_this_cycle = 0;
  store_queue.issued_this_cycle = 0;
  dtb.resetRequestCounter();
}

void CacheDatapath::registerStats()
{
  using namespace Stats;
  loads.name(accelerator_name + "_total_loads")
       .desc("Total number of dcache loads")
       .flags(total | nonan);
  stores.name(accelerator_name + "_total_stores")
        .desc("Total number of dcache stores.")
        .flags(total | nonan);
}

void
CacheDatapath::insertTLBEntry(Addr trace_addr, Addr vaddr, Addr paddr)
{
  DPRINTF(CacheDatapath, "Mapping trace_addr 0x%x, vaddr 0x%x -> paddr 0x%x.\n",
          trace_addr, vaddr, paddr);
  Addr vpn = vaddr & ~(dtb.pageMask());
  Addr ppn = paddr & ~(dtb.pageMask());
  DPRINTF(CacheDatapath, "Inserting TLB entry vpn 0x%x -> ppn 0x%x.\n",
          vpn, ppn);
  dtb.insert(vpn, ppn);
  dtb.insertTraceToVirtual(trace_addr, vaddr);
}

void CacheDatapath::event_step()
{
  step();
}

bool CacheDatapath::step() {
  // If the dependencies have not yet been fulfilled, do not proceed with
  // execution.
  if (system->numAcceleratorDepsRemaining(accelerator_id) > 0)
  {
    schedule(tickEvent, clockEdge(Cycles(1)));
    // Maybe add a counter for number of cycles spent waiting here.
    return true;
  }
  resetCacheCounters();
  stepExecutingQueue();
  copyToExecutingQueue();
  DPRINTF(CacheDatapath, "Cycles:%d, executedNodes:%d, totalConnectedNodes:%d\n",
     num_cycles, executedNodes, totalConnectedNodes);
  num_cycles++;
  if (executedNodes < totalConnectedNodes)
  {
    schedule(tickEvent, clockEdge(Cycles(1)));
    return true;
  }
  else
  {
    dumpStats();
    DPRINTF(CacheDatapath, "Accelerator completed.\n");
    if (execute_standalone) {
      system->deregisterAccelerator(accelerator_id);
      if (system->numRunningAccelerators() == 0) {
        exitSimLoop("Aladdin called exit()");
      }
    } else {
      sendFinishedSignal();
    }
  }
  return false;
}

void CacheDatapath::stepExecutingQueue()
{
  auto it = executingQueue.begin();
  int index = 0;
  while (it != executingQueue.end())
  {
    ExecNode* node = *it;
    unsigned node_id = node->get_node_id();
    if (node->is_memory_op())
    {
      MemAccess* mem_access = node->get_mem_access();
      InFlightMemAccess inflight_mem_op;
      inflight_mem_op.status = Ready;
      inflight_mem_op.paddr = 0x0;
      if (inflight_mem_ops.find(node_id) == inflight_mem_ops.end())
        inflight_mem_ops[node_id] = inflight_mem_op;
      else
        inflight_mem_op = inflight_mem_ops[node_id];
      bool isLoad = node->is_load_op();
      if (inflight_mem_op.status == Ready)
      {
        // First time seeing this node. Start the memory access procedure.
        Addr addr = mem_access->vaddr;
        int size = mem_access->size;
        if (dtb.canRequestTranslation() && accessTLB(addr, size, isLoad, node_id))
        {
          inflight_mem_ops[node_id].status = Translating;
          dtb.incrementRequestCounter();
          DPRINTF(CacheDatapath, "node:%d mem access is translating\n", node_id);
        }
        else if (!dtb.canRequestTranslation())
        {
          DPRINTF(CacheDatapath, "node:%d TLB queue is full\n", node_id);
        }
        ++it;
        ++index;
      }
      else if (inflight_mem_op.status == Translated)
      {
        Addr addr = inflight_mem_op.paddr;
        int size = mem_access->size;
        long long int value = mem_access->value;

        if (accessCache(addr, size, isLoad, node_id, value))
        {
          inflight_mem_ops[node_id].status = WaitingFromCache;
          if (isLoad) {
            load_queue.in_flight ++;
            loads ++;
          }
          else {
            store_queue.in_flight ++;
            stores ++;
          }
          DPRINTF(CacheDatapath, "node:%d mem access is accessing cache\n", node_id);
        }
        else
        {
          if (isLoad)
            DPRINTF(CacheDatapath, "node:%d load queue is full\n", node_id);
          else
            DPRINTF(CacheDatapath, "node:%d store queue is full\n", node_id);
        }
        ++it;
        ++index;
      }
      else if (inflight_mem_op.status == Returned)
      {
        if (isLoad)
          load_queue.in_flight --;
        else
          store_queue.in_flight --;
        inflight_mem_ops.erase(node_id);
        markNodeCompleted(it, index);
      }
      else
      {
        ++it;
        ++index;
      }
    }
    else
    {
      markNodeCompleted(it, index);
    }
  }
}

void CacheDatapath::dumpStats()
{
  computeCactiResults();
  BaseDatapath::dumpStats();
  BaseDatapath::writePerCycleActivity();
}

void CacheDatapath::computeCactiResults()
{
  DPRINTF(CacheDatapath,
          "Invoking CACTI for cache power and area estimates.\n");
  uca_org_t cacti_result = cacti_interface(cacti_cfg);
  readEnergy = cacti_result.power.readOp.dynamic * 1e9;
  writeEnergy = cacti_result.power.writeOp.dynamic * 1e9;
  leakagePower = cacti_result.power.readOp.leakage * 1000;
  area = cacti_result.area;

  dtb.computeCactiResults();
  load_queue.computeCactiResults();
  store_queue.computeCactiResults();
  cacti_result.cleanup();
}

/* Returns total memory power over the specified number of cycles. */
void CacheDatapath::getAverageMemPower(
    unsigned int cycles, float *avg_power, float *avg_dynamic, float *avg_leak)
{
  float avg_cache_pwr, avg_cache_leak, avg_cache_ac_pwr;
  float avg_lq_pwr, avg_lq_leak, avg_lq_ac_pwr;
  float avg_sq_pwr, avg_sq_leak, avg_sq_ac_pwr;
  float avg_tlb_pwr, avg_tlb_leak, avg_tlb_ac_pwr;

  avg_cache_ac_pwr = (loads.value() * readEnergy + stores.value() * writeEnergy) /
                      (cycles * cycleTime);
  avg_cache_leak = leakagePower;
  avg_cache_pwr = avg_cache_ac_pwr + avg_cache_leak;

  load_queue.getAveragePower(cycles, cycleTime, &avg_lq_pwr, &avg_lq_ac_pwr, &avg_lq_leak);
  store_queue.getAveragePower(cycles, cycleTime, &avg_sq_pwr, &avg_sq_ac_pwr, &avg_sq_leak);
  dtb.getAveragePower(cycles, cycleTime, &avg_tlb_pwr, &avg_tlb_ac_pwr, &avg_tlb_leak);

  *avg_dynamic = avg_cache_ac_pwr;
  *avg_leak = avg_cache_leak;
  *avg_power = avg_cache_pwr;

  // TODO: Write this detailed data to the database as well.
  std::string power_file_name = std::string(benchName) + "_power_stats.txt";
  std::ofstream power_file(power_file_name.c_str());

  power_file << "system.datapath.dcache.average_pwr " << avg_cache_pwr
             << " # Average dcache dynamic and leakage power." << std::endl;
  power_file << "system.datapath.dcache.dynamic_pwr " << avg_cache_ac_pwr
             << " # Average dcache dynamic power." << std::endl;
  power_file << "system.datapath.dcache.leakage_pwr " << avg_cache_leak
             << " # Average dcache leakage power." << std::endl;
  power_file << "system.datapath.dcache.area " << area
             << " # dcache area." << std::endl;

  power_file << "system.datapath.tlb.average_pwr " << avg_tlb_pwr
             << " # Average tlb dynamic and leakage power." << std::endl;
  power_file << "system.datapath.tlb.dynamic_pwr " << avg_tlb_ac_pwr
             << " # Average tlb dynamic power." << std::endl;
  power_file << "system.datapath.tlb.leakage_pwr " << avg_tlb_leak
             << " # Average tlb leakage power." << std::endl;
  power_file << "system.datapath.tlb.area " << dtb.getArea()
             << " # tlb area." << std::endl;

  power_file << "system.datapath.load_queue.average_pwr " << avg_lq_pwr
             << " # Average load queue dynamic and leakage power." << std::endl;
  power_file << "system.datapath.load_queue.dynamic_pwr " << avg_lq_ac_pwr
             << " # Average load queue dynamic power." << std::endl;
  power_file << "system.datapath.load_queue.leakage_pwr " << avg_lq_leak
             << " # Average load queue leakage power." << std::endl;
  power_file << "system.datapath.load_queue.area " << load_queue.getArea()
             << " # load_queue area." << std::endl;

  power_file << "system.datapath.store_queue.average_pwr " << avg_sq_pwr
             << " # Average store queue dynamic and leakage power." << std::endl;
  power_file << "system.datapath.store_queue.dynamic_pwr " << avg_sq_ac_pwr
             << " # Average store queue dynamic power." << std::endl;
  power_file << "system.datapath.store_queue.leakage_pwr " << avg_sq_leak
             << " # Average store queue leakage power." << std::endl;
  power_file << "system.datapath.store_queue.area " << store_queue.getArea()
             << " # store_queue area." << std::endl;

  power_file.close();
}

#ifdef USE_DB
int CacheDatapath::writeConfiguration(sql::Connection *con)
{
  int unrolling_factor, partition_factor;
  bool pipelining;
  getCommonConfigParameters(
      unrolling_factor, pipelining, partition_factor);

  sql::Statement *stmt = con->createStatement();
  stringstream query;
  query << "insert into configs (id, memory_type, trace_file, "
           "config_file, pipelining, unrolling, partitioning, "
           "cache_size, cache_line_sz, cache_assoc, cache_hit_latency, "
           "is_perfect_tlb, tlb_page_size, tlb_assoc, tlb_miss_latency, "
           "tlb_hit_latency, tlb_max_outstanding_walks, tlb_bandwidth, "
           "tlb_entries, load_queue_size, store_queue_size, load_bandwidth, "
           "store_bandwidth) values (";
  query << "NULL" << ",\"cache\"" << "," << "\"" << trace_file << "\"" << ",\""
        << config_file << "\"," << pipelining << "," << unrolling_factor << ","
        << partition_factor << ",\"" << cacheSize << "\"," << cacheLineSize
        << "," << cacheAssoc << "," << cacheHitLatency << ","
        << dtb.getIsPerfectTLB() << "," << dtb.getPageBytes() << ","
        << dtb.getAssoc() << "," << dtb.getMissLatency() << ","
        << dtb.getHitLatency() << "," << dtb.getNumOutStandingWalks() << ","
        << dtb.bandwidth << "," << dtb.getNumEntries() << "," << load_queue.size
        << "," << store_queue.size << "," << load_queue.bandwidth << ","
        << store_queue.bandwidth << ")";
  stmt->execute(query.str());
  delete stmt;
  return getLastInsertId(con);
}
#endif

void CacheDatapath::getMemoryBlocks(std::vector<std::string>& names) {}

void CacheDatapath::getRegisterBlocks(std::vector<std::string>& names) {}

double CacheDatapath::getTotalMemArea() { return area; }

float CacheDatapath::getReadEnergy( std::string block_name)
{
  return readEnergy;
}

float CacheDatapath::getWriteEnergy(std::string block_name)
{
  return writeEnergy;
}

float CacheDatapath::getLeakagePower(std::string block_name)
{
  return leakagePower;
}

float CacheDatapath::getArea(std::string block_name)
{
  return area;
}

Addr CacheDatapath::getBaseAddress(std::string label)
{
  return (Addr) BaseDatapath::getBaseAddress(label);
}
////////////////////////////////////////////////////////////////////////////
//
//  The SimObjects we use to get the Datapath information into the simulator
//
////////////////////////////////////////////////////////////////////////////

CacheDatapath* CacheDatapathParams::create()
{
  return new CacheDatapath(this);
}
