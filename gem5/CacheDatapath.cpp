/* Implementation of an accelerator datapath that uses a private scratchpad for
 * local memory.
 */

#include <string>

#include "debug/CacheDatapath.hh"
#include "CacheDatapath.h"

CacheDatapath::CacheDatapath(const Params *p) :
    BaseDatapath(
        p->benchName, p->traceFileName, p->configFileName, p->cycleTime),
    MemObject(p),
    _dataMasterId(p->system->getMasterId(name() + ".data")),
    dcachePort(this),
    tickEvent(this),
    retryPkt(NULL),
    isCacheBlocked(false),
    load_queue(p->loadQueueSize, p->loadBandwidth),
    store_queue(p->storeQueueSize, p->storeBandwidth),
    dtb(this,
        p->tlbEntries,
        p->tlbAssoc,
        p->tlbHitLatency,
        p->tlbMissLatency,
        p->tlbPageBytes,
        p->isPerfectTLB,
        p->numOutStandingWalks,
        p->tlbBandwidth),
    inFlightNodes(0),
    system(p->system)
{
  initActualAddress();
  setGlobalGraph();
  globalOptimizationPass();
  setGraphForStepping();
  num_cycles = 0;
  system->registerAcceleratorStart();
  schedule(tickEvent, clockEdge(Cycles(1)));
}

CacheDatapath::~CacheDatapath() {}

BaseMasterPort &
CacheDatapath::getMasterPort(const string &if_name,
                                          PortID idx)
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
  DPRINTF(CacheDatapath, "recvTimingResp for address: %#x %s\n", pkt->getAddr(), pkt->cmdString());
  if (pkt->isError())
    DPRINTF(CacheDatapath, "Got error packet back for address: %#x\n", pkt->getAddr());
  datapath->completeDataAccess(pkt);
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
  assert(datapath->isCacheBlocked && datapath->retryPkt != NULL );
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
  DPRINTF(CacheDatapath, "completeDataAccess for addr:%#x %s\n", pkt->getAddr(), pkt->cmdString());
  DatapathSenderState *state = dynamic_cast<DatapathSenderState *> (pkt->senderState);

  //Mark nodes ready to fire
  unsigned node_id = state->node_id;
  assert(mem_accesses.find(node_id) != mem_accesses.end());
  assert(mem_accesses[node_id] == WaitingFromCache);
  mem_accesses[node_id] = Returned;
  DPRINTF(CacheDatapath, "node:%d mem access is returned\n", node_id);
  delete state;
  delete pkt->req;
  delete pkt;
}

void
CacheDatapath::finishTranslation(PacketPtr pkt, bool was_miss)
{
  DPRINTF(CacheDatapath, "finishTranslation for addr:%#x %s\n", pkt->getAddr(), pkt->cmdString());
  DatapathSenderState *state = dynamic_cast<DatapathSenderState *> (pkt->senderState);
  unsigned node_id = state->node_id;
  assert(mem_accesses.find(node_id) != mem_accesses.end());
  assert(mem_accesses[node_id] == Translating);
  /* If the TLB missed, we need to retry the complete instruction, as the dcache
   * state may have changed during the waiting time, so we go back to Ready
   * instead of Translated. The next translation request should hit.
   */
  if (was_miss)
    mem_accesses[node_id] = Ready;
  else
    mem_accesses[node_id] = Translated;
  DPRINTF(CacheDatapath, "node:%d mem access is translated\n", node_id);
  delete state;
  delete pkt->req;
  delete pkt;
}

bool
CacheDatapath::accessTLB (Addr addr, unsigned size, bool isLoad, int node_id)
{
  DPRINTF(CacheDatapath, "accessTLB for addr:%#x\n", addr);
  //form request
  Request *req = NULL;
  //physical request
  Flags<FlagsType> flags = 0;
  //constructor for physical request only
  req = new Request (addr, size, flags, dataMasterId());

  MemCmd command;
  if (isLoad)
    command = MemCmd::ReadReq;
  else
    command = MemCmd::WriteReq;
  PacketPtr data_pkt = new Packet(req, command);

  uint8_t *data = new uint8_t[64];
  data_pkt->dataStatic<uint8_t>(data);

  DatapathSenderState *state = new DatapathSenderState(node_id);
  data_pkt->senderState = state;

  //TLB access
  if (dtb.translateTiming(data_pkt))
    return true;
  else
  {
    delete state;
    delete data_pkt->req;
    delete data_pkt;
    return false;
  }
}

bool
CacheDatapath::accessCache(Addr addr, unsigned size, bool isLoad, int node_id)
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
  if (isLoad)
    load_queue.issued_this_cycle ++;
  else
    store_queue.issued_this_cycle ++;

  //form request
  Request *req = NULL;
  //physical request
  Flags<FlagsType> flags = 0;
  //constructor for physical request only
  req = new Request (addr, size, flags, dataMasterId());

  MemCmd command;
  if (isLoad)
    command = MemCmd::ReadReq;
  else
    command = MemCmd::WriteReq;
  PacketPtr data_pkt = new Packet(req, command);

  uint8_t *data = new uint8_t[64];
  data_pkt->dataStatic<uint8_t>(data);

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
      DPRINTF(CacheDatapath, "load issued to dcache!\n");
    else
      DPRINTF(CacheDatapath, "store issued to dcache!\n");

  }

  return true;
}

void CacheDatapath::globalOptimizationPass()
{
  // Node removals must come first.
  removeInductionDependence();
  removePhiNodes();
  // Base address must be initialized next.
  loopFlatten();
  loopUnrolling();
  removeSharedLoads();
  storeBuffer();
  removeRepeatedStores();
  treeHeightReduction();
  // Must do loop pipelining last; after all the data/control dependences are fixed
  loopPipelining();
}

void CacheDatapath::initActualAddress()
{
  ostringstream file_name;
  file_name << benchName << "_memaddr.gz";
  gzFile gzip_file;
  gzip_file = gzopen(file_name.str().c_str(), "r");
  while (!gzeof(gzip_file))
  {
    char buffer[256];
    if (gzgets(gzip_file, buffer, 256) == NULL)
      break;
    unsigned node_id;
    long long int address;
    int size;
    sscanf(buffer, "%d,%lld,%d\n", &node_id, &address, &size);

    actualAddress[node_id] = make_pair(address & MASK, size/8);
  }
  gzclose(gzip_file);
}

void CacheDatapath::copyToExecutingQueue()
{
  auto it = readyToExecuteQueue.begin();
  while (it != readyToExecuteQueue.end())
  {
    executingQueue.push_back(*it);
    it = readyToExecuteQueue.erase(it);
  }
}

void CacheDatapath::resetCacheCounters()
{
  load_queue.issued_this_cycle = 0;
  store_queue.issued_this_cycle = 0;
  dtb.resetRequestCounter();
}
void CacheDatapath::event_step()
{
  step();
}

bool CacheDatapath::step() {
  resetCacheCounters();
  stepExecutingQueue();
  copyToExecutingQueue();
  DPRINTF(CacheDatapath, "Aladdin stepping @ Cycle:%d, executed:%d, total:%d\n", num_cycles, executedNodes, totalConnectedNodes);
  num_cycles++;
  //FIXME: exit condition
  if (executedNodes < totalConnectedNodes)
  {
    schedule(tickEvent, clockEdge(Cycles(1)));
    return true;
  }
  else
  {
    dumpStats();
    system->registerAcceleratorExit();
    if (system->totalNumInsts == 0 && //no cpu
        system->numRunningAccelerators() == 0)
    {
      exitSimLoop("Aladdin called exit()");
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
    unsigned node_id = *it;
    if (is_memory_op(microop.at(node_id)))
    {
      MemAccessStatus status = Ready;
      if (mem_accesses.find(node_id) == mem_accesses.end())
        mem_accesses[node_id] = status;
      else
        status = mem_accesses[node_id];
      bool isLoad = is_load_op(microop.at(node_id));
      if (status == Ready)
      {
        // First time seeing this node. Start the memory access procedure.
        Addr addr = actualAddress[node_id].first;
        int size = actualAddress[node_id].second / 16;
        if (dtb.canRequestTranslation() && accessTLB(addr, size, isLoad, node_id))
        {
          mem_accesses[node_id] = Translating;
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
      else if (status == Translated)
      {
        Addr addr = actualAddress[node_id].first;
        int size = actualAddress[node_id].second / 16;
        if (accessCache(addr, size, isLoad, node_id))
        {
          mem_accesses[node_id] = WaitingFromCache;
          if (isLoad)
            load_queue.in_flight ++;
          else
            store_queue.in_flight ++;
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
      else if (status == Returned)
      {
        if (isLoad)
          load_queue.in_flight --;
        else
          store_queue.in_flight --;
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

void CacheDatapath::dumpStats() {
  writeTLBStats();
  BaseDatapath::dumpStats();
  writePerCycleActivity(nullptr);
}

void CacheDatapath::writeTLBStats()
{
  std::string bn(benchName);

  ofstream tlb_stats;
  std::string tmp_name = bn + "_tlb_stats";
  tlb_stats.open(tmp_name.c_str());
  tlb_stats << "system.datapath.tlb.hits " << dtb.hits << " # number of TLB hits" << std::endl;
  tlb_stats << "system.datapath.tlb.misses " << dtb.misses << " # number of TLB misses" << std::endl;
  tlb_stats << "system.datapath.tlb.hitRate " << dtb.hits * 1.0 / (dtb.hits + dtb.misses) << " # hit rate for Aladdin TLB" << std::endl;
  tlb_stats.close();
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
