// Implementation of a hybrid datapath with both caches and scratchpads.

#include <cstdint>
#include <functional>  // For std::hash
#include <sstream>
#include <string>

#include "base/flags.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "base/misc.hh"
#include "dev/dma_device.hh"
#include "mem/mem_object.hh"
#include "mem/packet.hh"
#include "mem/request.hh"
#include "sim/clocked_object.hh"
#include "sim/sim_exit.hh"
#include "sim/system.hh"

#include "aladdin/common/DatabaseDeps.h"
#include "aladdin/common/ExecNode.h"
#include "aladdin/common/ScratchpadDatapath.h"
#include "debug/HybridDatapath.hh"
#include "aladdin_tlb.hh"
#include "HybridDatapath.h"

HybridDatapath::HybridDatapath(
    const HybridDatapathParams* params) :
    ScratchpadDatapath(params->benchName,
                       params->traceFileName,
                       params->configFileName,
                       params->spadPorts),
    Gem5Datapath(params,
                 params->acceleratorId,
                 params->executeStandalone,
                 params->system),
    dmaSetupLatency(params->dmaSetupLatency),
    spadPort(this, params->system, params->maxDmaRequests),
    spadMasterId(params->system->getMasterId(name() + ".spad")),
    cachePort(this),
    cacheMasterId(params->system->getMasterId(name() + ".cache")),
    load_queue(params->loadQueueSize,
               params->loadBandwidth,
               params->acceleratorName + ".load_queue",
               params->loadQueueCacheConfig),
    store_queue(params->storeQueueSize,
                params->storeBandwidth,
                params->acceleratorName + ".store_queue",
                params->storeQueueCacheConfig),
    cacheSize(params->cacheSize),
    cacti_cfg(params->cactiCacheConfig),
    cacheLineSize(params->cacheLineSize),
    cacheHitLatency(params->cacheHitLatency),
    cacheAssoc(params->cacheAssoc),
    cache_readEnergy(0),
    cache_writeEnergy(0),
    cache_leakagePower(0),
    cache_area(0),
    dtb(this,
        params->tlbEntries,
        params->tlbAssoc,
        params->tlbHitLatency,
        params->tlbMissLatency,
        params->tlbPageBytes,
        params->isPerfectTLB,
        params->numOutStandingWalks,
        params->tlbBandwidth,
        params->tlbCactiConfig,
        params->acceleratorName),
    tickEvent(this),
    executedNodesLastTrigger(0)
{
  BaseDatapath::use_db = params->useDb;
  BaseDatapath::experiment_name = params->experimentName;
#ifdef USE_DB
  if (use_db) {
    BaseDatapath::setExperimentParameters(experiment_name);
  }
#endif
  BaseDatapath::cycleTime = params->cycleTime;
  std::stringstream name_builder;
  name_builder << "datapath" << accelerator_id;
  datapath_name = name_builder.str();
  system->registerAccelerator(accelerator_id, this, accelerator_deps);
  if (execute_standalone)
    initializeDatapath();
}

HybridDatapath::~HybridDatapath() {
  if (!execute_standalone)
    system->deregisterAccelerator(accelerator_id);
}

void HybridDatapath::clearDatapath(bool flush_tlb) {
  ScratchpadDatapath::clearDatapath();
  dtb.resetCounters();
  load_queue.resetCounters();
  store_queue.resetCounters();
  resetCounters(flush_tlb);
}

void HybridDatapath::resetCounters(bool flush_tlb) {
  loads = 0;
  stores = 0;
  if (flush_tlb)
    dtb.clear();
}

void HybridDatapath::clearDatapath() {
  clearDatapath(false);
}


void HybridDatapath::initializeDatapath(int delay) {
  buildDddg();
  globalOptimizationPass();
  prepareForScheduling();
  num_cycles = 0;
  startDatapathScheduling(delay);
}

void HybridDatapath::startDatapathScheduling(int delay) {
  scheduleOnEventQueue(delay);
}

BaseMasterPort&
HybridDatapath::getMasterPort(const string &if_name, PortID idx)
{
  // Get the right port based on name. This applies to all the
  // subclasses of the base CPU and relies on their implementation
  // of getDataPort and getInstPort. In all cases there methods
  // return a MasterPort pointer.
  if (if_name == "spad_port")
    return getDataPort();
  else if (if_name == "cache_port")
    return getCachePort();
  else
    return MemObject::getMasterPort(if_name);
}

void
HybridDatapath::insertTLBEntry(Addr vaddr, Addr paddr)
{
  DPRINTF(HybridDatapath,
          "Mapping vaddr 0x%x -> paddr 0x%x.\n",
          vaddr,
          paddr);
  Addr vpn = vaddr & ~(dtb.pageMask());
  Addr ppn = paddr & ~(dtb.pageMask());
  DPRINTF(
      HybridDatapath, "Inserting TLB entry vpn 0x%x -> ppn 0x%x.\n", vpn, ppn);
  dtb.insertBackupTLB(vpn, ppn);
}

void
HybridDatapath::insertArrayLabelToVirtual(std::string array_label, Addr vaddr)
{
  DPRINTF(
      HybridDatapath, "Inserting array label mapping %s -> vpn 0x%x.\n",
      array_label.c_str(), vaddr);
  dtb.insertArrayLabelToVirtual(array_label, vaddr);
}

void
HybridDatapath::event_step()
{
  step();
  scratchpad->step();
  printf_guards.write_buffered_output();
  printf_guards.reset();
}

void
HybridDatapath::stepExecutingQueue()
{
  auto it = executingQueue.begin();
  int index = 0;
  while (it != executingQueue.end()) {
    ExecNode* node = *it;
    bool op_satisfied = false;
    if (node->is_memory_op() || node->is_dma_op()) {
      MemoryOpType type = getMemoryOpType(node);
      switch (type) {
        case Register:
          op_satisfied = handleRegisterMemoryOp(node);
          break;
        case Scratchpad:
          op_satisfied = handleSpadMemoryOp(node);
          break;
        case Cache:
          op_satisfied = handleCacheMemoryOp(node);
          break;
        case Dma:
          op_satisfied = handleDmaMemoryOp(node);
          break;
      }
      if (op_satisfied) {
        markNodeCompleted(it, index);
      }
    } else if (node->is_multicycle_op()) {
      unsigned node_id = node->get_node_id();
      if (inflight_multicycle_nodes.find(node_id)
            == inflight_multicycle_nodes.end()) {
        inflight_multicycle_nodes[node_id] = node->get_multicycle_latency();
        markNodeStarted(node);
      } else {
        unsigned remaining_cycles = inflight_multicycle_nodes[node_id];
        if (remaining_cycles == 1) {
          inflight_multicycle_nodes.erase(node_id);
          markNodeCompleted(it, index);
          op_satisfied = true;
        } else {
          inflight_multicycle_nodes[node_id]--;
        }
      }
    } else {
      // Not a memory/fp operation node, so it can be completed in one cycle.
      markNodeStarted(node);
      markNodeCompleted(it, index);
      op_satisfied = true;
    }
    if (!op_satisfied) {
        ++it;
        ++index;
    }
  }
}

void
HybridDatapath::retireReturnedLSQEntries()
{
  load_queue.retireReturnedEntries();
  store_queue.retireReturnedEntries();
}

bool
HybridDatapath::step()
{
  executedNodesLastTrigger = executedNodes;
  resetCacheCounters();
  stepExecutingQueue();
  copyToExecutingQueue();
  retireReturnedLSQEntries();
  DPRINTF(HybridDatapath,
          "[CYCLES]:%d. executedNodes:%d, totalConnectedNodes:%d\n",
          num_cycles,
          executedNodes,
          totalConnectedNodes);
  if (executedNodesLastTrigger != executedNodes)
    cycles_since_last_node++;
  else
    cycles_since_last_node = 0;
  if (cycles_since_last_node > deadlock_threshold) {
    exitSimLoop("Detected deadlock!");
    return false;
  }

  num_cycles++;
  if (executedNodes < totalConnectedNodes) {
    schedule(tickEvent, clockEdge(Cycles(1)));
    return true;
  } else {
    dumpStats();
    // TODO: For multiple invocations, there's no purpose in flushing the TLB
    // in between invocations.
    clearDatapath(false);
    DPRINTF(HybridDatapath, "Accelerator completed.\n");
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

HybridDatapath::MemoryOpType
HybridDatapath::getMemoryOpType(ExecNode* node)
{
  std::string array_label = node->get_array_label();
  if (node->is_dma_op())
    return Dma;
  else if (registers.has(array_label))
    return Register;
  else if (scratchpad->partitionExist(array_label))
    return Scratchpad;
  else
    return Cache;
}

bool
HybridDatapath::handleRegisterMemoryOp(ExecNode* node)
{
  markNodeStarted(node);
  std::string reg = node->get_array_label();
  if (node->is_load_op())
    registers.getRegister(reg)->increment_loads();
  else
    registers.getRegister(reg)->increment_stores();
  return true;
}

bool
HybridDatapath::handleSpadMemoryOp(ExecNode* node)
{
  std::string node_part = node->get_array_label();
  bool satisfied = scratchpad->addressRequest(node_part);
  if (!satisfied)
    return false;

  markNodeStarted(node);
  if (node->is_load_op())
    scratchpad->increment_loads(node_part);
  else
    scratchpad->increment_stores(node_part);
  return true;
}

bool
HybridDatapath::handleDmaMemoryOp(ExecNode* node)
{
  MemAccessStatus status = Ready;
  unsigned node_id = node->get_node_id();
  if (inflight_mem_nodes.find(node_id) == inflight_mem_nodes.end())
    inflight_mem_nodes[node_id] = status;
  else
    status = inflight_mem_nodes[node_id];
  if (status == Ready && inflight_mem_nodes.size() < MAX_INFLIGHT_NODES) {
    //first time see, do access
    markNodeStarted(node);
    bool isLoad = node->is_dma_load();
    /* TODO: HACK!!! dmaStore sometimes goes out-of-order...schedule it until
     * the very end, fix this after ISCA. --Sophia */
    if (isLoad || executedNodes >= totalConnectedNodes-5) {
      MemAccess* mem_access = node->get_mem_access();
      unsigned size = mem_access->size;
      Addr vaddr = mem_access->vaddr;
      issueDmaRequest(vaddr, size, isLoad, node->get_node_id());
      inflight_mem_nodes[node_id] = WaitingFromDma;
      DPRINTF(
          HybridDatapath, "node:%d is a dma request\n", node->get_node_id());
    }
    return false; // DMA op not completed. Move on to the next node.
  } else if (status == Returned) {
    inflight_mem_nodes.erase(node_id);
    return true;  // DMA op completed.
  } else {
    // Still waiting from the cache. Move on to the next node.
    return false;
  }
}

bool HybridDatapath::handleCacheMemoryOp(ExecNode* node) {
  MemAccessStatus inflight_mem_op = Ready;
  MemAccess* mem_access = node->get_mem_access();
  unsigned node_id = node->get_node_id();
  bool isLoad = node->is_load_op();

  // If Aladdin is executing standalone, then the actual addresses that are
  // sent to the memory system of gem5 don't matter as long as there is a 1:1
  // correspondence between them and those in the trace. This mask ensures
  // that even if a trace was generated on a 64 bit system, gem5 will not
  // attempt to access an address outside of a 32-bit address space.
  Addr vaddr = mem_access->vaddr;
  if (isExecuteStandalone())
    vaddr &= ADDR_MASK;

  MemoryQueue& queue = isLoad ? load_queue : store_queue;
  Stats::Scalar& mem_stat = isLoad ? loads : stores;

  if (queue.contains(vaddr))
    inflight_mem_op = queue.getStatus(vaddr);

  if (inflight_mem_op == Ready) {
    // This is either the first time we're seeing this node, or we are retrying
    // this node after a TLB miss. We'll enqueue it - if it already exists in
    // the queue, a duplicate won't be added.
    if (queue.contains(vaddr)) {
      if (printf_guards.lsq_merge_count < printf_guards.threshold)
        DPRINTF(HybridDatapath,
                "node:%d %s was merged into an existing entry.\n",
                node_id, isLoad ? "load" : "store");
      printf_guards.lsq_merge_count++;
    } else if (!queue.is_full()) {
      queue.enqueue(vaddr);
      mem_stat++;
    } else {
      if (printf_guards.lsq_full_count < printf_guards.threshold)
        DPRINTF(HybridDatapath, "node:%d %s queue is full\n",
                node_id, isLoad ?  "load" : "store");
      printf_guards.lsq_full_count++;
      return false;
    }

    int size = mem_access->size;
    if (dtb.canRequestTranslation() &&
        issueTLBRequest(vaddr, size, isLoad, node_id)) {
      markNodeStarted(node);
      dtb.incrementRequestCounter();
      queue.setStatus(vaddr, Translating);
      DPRINTF(HybridDatapath, "node:%d, vaddr = %x, is translating\n", node_id,
      vaddr);
    } else if (!dtb.canRequestTranslation()) {
      if (printf_guards.tlb_bw_count < printf_guards.threshold)
        DPRINTF(HybridDatapath,
                "node:%d TLB cannot accept any more requests\n",
                node_id);
      printf_guards.tlb_bw_count++;
    }
    return false;
  } else if (inflight_mem_op == Translated) {
    Addr paddr = mem_access->paddr;
    int size = mem_access->size;
    long long int value = mem_access->value;

    if (issueCacheRequest(paddr, size, isLoad, node_id, value)) {
      queue.setStatus(vaddr, WaitingFromCache);
      DPRINTF(
          HybridDatapath, "node:%d, vaddr = 0x%x, paddr = 0x%x is accessing cache\n",
                           node_id, vaddr, paddr);
    }
    return false;
  } else if (inflight_mem_op == Returned) {
    // Load store queue entries that have returned will be deleted at the end
    // of the cycle.
    return true;
  } else {
    return false;
  }
}

/* Mark the DMA request node as having completed. */
void
HybridDatapath::completeDmaRequest(unsigned node_id)
{
  DPRINTF(HybridDatapath,
          "completeDmaRequest for addr:%#x \n",
          exec_nodes[node_id]->get_mem_access()->vaddr);
  inflight_mem_nodes[node_id] = Returned;
}

// Issue a DMA request for memory.
void
HybridDatapath::issueDmaRequest(
    Addr addr, unsigned size, bool isLoad, unsigned node_id)
{
  DPRINTF(
      HybridDatapath, "issueDmaRequest for addr:%#x, size:%u\n", addr, size);
  addr &= ADDR_MASK;
  MemCmd::Command cmd = isLoad ? MemCmd::ReadReq : MemCmd::WriteReq;
  Request::Flags flag = 0;
  uint8_t *data = new uint8_t[size];
  dmaQueue.push_back(node_id);
  DmaEvent *dma_event = new DmaEvent(this);
  spadPort.dmaAction(
      cmd, addr, size, dma_event, data, Cycles(dmaSetupLatency), flag);
}

void
HybridDatapath::sendFinishedSignal()
{
  Flags<Packet::FlagsType> flags = 0;
  size_t size = 4;  // 32 bit integer.
  uint8_t *data = new uint8_t[size];
  // Set some sentinel value.
  for (int i = 0; i < size; i++)
    data[i] = 0x01;
  Request *req = new Request(finish_flag, size, flags, getCacheMasterId());
  req->setThreadContext(context_id, thread_id);  // Only needed for prefetching.
  MemCmd::Command cmd = MemCmd::WriteReq;
  PacketPtr pkt = new Packet(req, cmd);
  pkt->dataStatic<uint8_t>(data);
  DatapathSenderState *state = new DatapathSenderState(-1, true);
  pkt->senderState = state;

  if (!cachePort.sendTimingReq(pkt))
  {
    assert(retryPkt == NULL);
    retryPkt = pkt;
    DPRINTF(HybridDatapath,
            "Sending finished signal failed, retrying.\n");
  }
  else
  {
    DPRINTF(HybridDatapath, "Sent finished signal.\n");
  }
}

void
HybridDatapath::CachePort::recvRetry()
{
  DPRINTF(HybridDatapath, "recvRetry for addr:");
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

  DPRINTF(HybridDatapath, "%#x \n", datapath->retryPkt->getAddr());
  if (datapath->cachePort.sendTimingReq(datapath->retryPkt))
  {
    DPRINTF(HybridDatapath, "Retry pass!\n");
    datapath->retryPkt = NULL;
    datapath->isCacheBlocked = false;
  }
  else
    DPRINTF(HybridDatapath, "Still blocked!\n");
}

bool
HybridDatapath::CachePort::recvTimingResp(PacketPtr pkt)
{
  DPRINTF(HybridDatapath,
          "recvTimingResp for address: %#x %s\n",
          pkt->getAddr(),
          pkt->cmdString());
  if (pkt->isError()) {
    DPRINTF(HybridDatapath,
            "Got error packet back for address: %#x\n",
            pkt->getAddr());
  }
  DatapathSenderState* state =
      dynamic_cast<DatapathSenderState*>(pkt->senderState);
  if (!state->is_ctrl_signal) {
    datapath->completeCacheRequest(pkt);
  } else {
    DPRINTF(HybridDatapath,
            "recvTimingResp for control signal access: %#x\n",
            pkt->getAddr());
    delete state;
    delete pkt->req;
    delete pkt;
  }
  return true;
}

bool
HybridDatapath::issueTLBRequest(Addr addr,
                                unsigned size,
                                bool isLoad,
                                unsigned node_id) {
  DPRINTF(HybridDatapath, "issueTLBRequest for trace addr:%#x\n", addr);
  Request *req = NULL;
  Flags<Packet::FlagsType> flags = 0;
  // Constructor for physical request only
  req = new Request (addr, size/8, flags, getCacheMasterId());

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

void
HybridDatapath::completeTLBRequest(PacketPtr pkt, bool was_miss)
{
  AladdinTLB::TLBSenderState *state =
      dynamic_cast<AladdinTLB::TLBSenderState*>(pkt->senderState);
  DPRINTF(HybridDatapath,
          "completeTLBRequest for addr:%#x %s\n",
          pkt->getAddr(),
          pkt->cmdString());
  Addr vaddr = pkt->getAddr();
  unsigned node_id = state->node_id;
  ExecNode* node = exec_nodes[node_id];
  MemoryQueue& queue = node->is_load_op() ? load_queue: store_queue;
  assert(queue.contains(vaddr) &&
         queue.getStatus(vaddr) == Translating);

  /* If the TLB missed, we need to retry the complete instruction, as the dcache
   * state may have changed during the waiting time, so we go back to Ready
   * instead of Translated. The next translation request should hit.
   */
  if (was_miss) {
    queue.setStatus(vaddr, Ready);
  } else {
    // Translations are actually the complete memory address, not just the page
    // number, because of the trace to virtual address translation.
    Addr paddr = *pkt->getPtr<Addr>();
    MemAccess* mem_access = exec_nodes[node_id]->get_mem_access();
    mem_access->paddr = paddr;
    queue.setStatus(vaddr, Translated);
    queue.setPhysicalAddress(vaddr, paddr);
    DPRINTF(HybridDatapath, "node:%d was translated, vaddr = %x, paddr = %x\n",
                             node_id, vaddr, paddr);
  }
  delete state;
  delete pkt->req;
  delete pkt;
}

bool HybridDatapath::issueCacheRequest(Addr addr,
                                       unsigned size,
                                       bool isLoad,
                                       unsigned node_id,
                                       long long int value) {
  DPRINTF(HybridDatapath, "issueCacheRequest for addr:%#x\n", addr);
  bool queues_available = (isLoad && load_queue.can_issue()) ||
                          (!isLoad && store_queue.can_issue());
  if (!queues_available) {
    if (printf_guards.lsq_ports_count < printf_guards.threshold)
      DPRINTF(HybridDatapath,
              "Load/store queues have no more ports available.\n");
    printf_guards.lsq_ports_count++;
    return false;
  }
  if (isCacheBlocked) {
    DPRINTF(HybridDatapath, "MSHR is full. Waiting for misses to return.\n");
    return false;
  }

  // Bandwidth limitations.
  if (isLoad) {
    load_queue.issued_this_cycle++;
    load_queue.writeStats++;
  } else {
    store_queue.issued_this_cycle++;
    store_queue.writeStats++;
  }

  Request* req = NULL;
  Flags<Packet::FlagsType> flags = 0;
  /* To use strided prefetching, we need to include a "PC" so the prefetcher
   * can predict memory behavior similar to how branch predictors work. We
   * don't have real PCs in aladdin so we'll just hash the unique id of the
   * node.  */
  std::string unique_id = exec_nodes[node_id]->get_static_node_id();
  Addr pc = static_cast<Addr>(std::hash<std::string>()(unique_id));
  req = new Request(addr, size / 8, flags, getCacheMasterId(), curTick(), pc);
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

  if (!cachePort.sendTimingReq(data_pkt)) {
    // blocked, retry
    assert(retryPkt == NULL);
    retryPkt = data_pkt;
    isCacheBlocked = true;
    DPRINTF(HybridDatapath, "port is blocked. Will retry later...\n");
  } else {
    if (isLoad)
      DPRINTF(HybridDatapath,
              "Node id %d load from address %#x issued to "
              "dcache!\n",
              node_id,
              addr);
    else
      DPRINTF(HybridDatapath,
              "Node id %d store of value %d to address %#x "
              "issued to dcache!\n",
              node_id,
              value,
              addr);
  }

  return true;
}

/* Data that is returned gets written back into the load store queues.
 * Note that due to our trace driven execution framework, we don't actually
 * care about the dynamic data returned by a load. We only care that a store
 * successfully writes the value into the cache.
 */
void
HybridDatapath::completeCacheRequest(PacketPtr pkt)
{
  DatapathSenderState* state =
      dynamic_cast<DatapathSenderState*>(pkt->senderState);
  unsigned node_id = state->node_id;
  Addr paddr = pkt->getAddr();  // Packet address is physical.
  DPRINTF(HybridDatapath,
          "completeCacheRequest for node_id %d, addr:%#x %s\n",
          node_id,
          paddr,
          pkt->cmdString());
  DPRINTF(HybridDatapath, "node:%d mem access is returned\n", node_id);

  // TODO: Queues are keyed by virtual addresses. We need a lookup
  // function for the queues in order to properly model this behavior. Need to
  // look up some more details on how load store queues are implemented.
  bool isLoad = (pkt->cmd == MemCmd::ReadResp);
  MemoryQueue& queue = isLoad ? load_queue : store_queue;
  MemAccess* mem_access = exec_nodes[node_id]->get_mem_access();
  Addr vaddr = mem_access->vaddr;
  if (isExecuteStandalone())
    vaddr &= ADDR_MASK;
  queue.setStatus(vaddr, Returned);
  queue.writeStats++;
  DPRINTF(HybridDatapath,
          "setting %s queue entry for address %#x to Returned.\n",
          isLoad ? "load" : "store",
          vaddr);

  delete state;
  delete pkt->req;
  delete pkt;
}

bool
HybridDatapath::SpadPort::recvTimingResp(PacketPtr pkt)
{
  return DmaPort::recvTimingResp(pkt);
}

HybridDatapath::DmaEvent::DmaEvent(HybridDatapath *_dpath)
  : Event(Default_Pri, AutoDelete), datapath(_dpath) {}

void
HybridDatapath::DmaEvent::process()
{
  assert(!datapath->dmaQueue.empty());
  datapath->completeDmaRequest(datapath->dmaQueue.front());
  datapath->dmaQueue.pop_front();
}

const char*
HybridDatapath::DmaEvent::description() const
{
  return "Hybrid DMA datapath receving request event";
}

void
HybridDatapath::resetCacheCounters()
{
  load_queue.issued_this_cycle = 0;
  store_queue.issued_this_cycle = 0;
  dtb.resetRequestCounter();
}

void
HybridDatapath::registerStats()
{
  using namespace Stats;
  loads.name(datapath_name + "_total_loads")
       .desc("Total number of dcache loads")
       .flags(total | nonan);
  stores.name(datapath_name + "_total_stores")
        .desc("Total number of dcache stores.")
        .flags(total | nonan);
}

#ifdef USE_DB
int
HybridDatapath::writeConfiguration(sql::Connection *con)
{
  int unrolling_factor, partition_factor;
  bool pipelining;
  getCommonConfigParameters(unrolling_factor, pipelining, partition_factor);

  sql::Statement *stmt = con->createStatement();
  stringstream query;
  query << "insert into configs (id, memory_type, trace_file, "
           "config_file, pipelining, unrolling, partitioning, "
           "max_dma_requests, dma_setup_latency, cache_size, cache_line_sz, "
           "cache_assoc, cache_hit_latency, "
           "is_perfect_tlb, tlb_page_size, tlb_assoc, tlb_miss_latency, "
           "tlb_hit_latency, tlb_max_outstanding_walks, tlb_bandwidth, "
           "tlb_entries, load_queue_size, store_queue_size, load_bandwidth, "
           "store_bandwidth) values (";
  query << "NULL"
        << ",\"spad\""
        << ","
        << "\"" << trace_file << "\""
        << ",\"" << config_file << "\"," << pipelining << ","
        << unrolling_factor << "," << partition_factor << ","
        << spadPort.max_req << "," << dmaSetupLatency << ",\"" << cacheSize
        << "\"," << cacheLineSize << "," << cacheAssoc << "," << cacheHitLatency
        << "," << dtb.getIsPerfectTLB() << "," << dtb.getPageBytes() << ","
        << dtb.getAssoc() << "," << dtb.getMissLatency() << ","
        << dtb.getHitLatency() << "," << dtb.getNumOutStandingWalks() << ","
        << dtb.bandwidth << "," << dtb.getNumEntries() << "," << load_queue.size
        << "," << store_queue.size << "," << load_queue.bandwidth << ","
        << store_queue.bandwidth << ")";
  stmt->execute(query.str());
  delete stmt;
  // Get the newly added config_id.
  return getLastInsertId(con);
}
#endif

void HybridDatapath::computeCactiResults()
{
  // If no caches wer used, then we don't need to invoke CACTI.
  if (cacti_cfg.empty())
    return;
  DPRINTF(HybridDatapath,
          "Invoking CACTI for cache power and area estimates.\n");
  uca_org_t cacti_result = cacti_interface(cacti_cfg);
  // power in mW, energy in pJ, area in mm2
  cache_readEnergy = cacti_result.power.readOp.dynamic * 1e12;
  cache_writeEnergy = cacti_result.power.writeOp.dynamic * 1e12;
  cache_leakagePower = cacti_result.power.readOp.leakage * 1000;
  cache_area = cacti_result.area;

  dtb.computeCactiResults();
  load_queue.computeCactiResults();
  store_queue.computeCactiResults();
  cacti_result.cleanup();
}

void
HybridDatapath::getAverageMemPower(
    unsigned int cycles, float *avg_power, float *avg_dynamic, float *avg_leak)
{
  float cache_avg_power = 0, cache_avg_dynamic = 0, cache_avg_leak = 0;
  float spad_avg_power = 0, spad_avg_dynamic = 0, spad_avg_leak = 0;
  computeCactiResults();
  getAverageCachePower(
      cycles, &cache_avg_power, &cache_avg_dynamic, &cache_avg_leak);
  getAverageSpadPower(
      cycles, &spad_avg_power, &spad_avg_dynamic, &spad_avg_leak);
  *avg_power = cache_avg_power + spad_avg_power;
  *avg_dynamic = cache_avg_dynamic + spad_avg_dynamic;
  *avg_leak = cache_avg_leak + spad_avg_leak;
}

void
HybridDatapath::getAverageCachePower(
    unsigned int cycles, float *avg_power, float *avg_dynamic, float *avg_leak)
{
  float avg_cache_pwr, avg_cache_leak, avg_cache_ac_pwr;
  float avg_lq_pwr, avg_lq_leak, avg_lq_ac_pwr;
  float avg_sq_pwr, avg_sq_leak, avg_sq_ac_pwr;
  float avg_tlb_pwr, avg_tlb_leak, avg_tlb_ac_pwr;

  avg_cache_ac_pwr =
      (loads.value() * cache_readEnergy + stores.value() * cache_writeEnergy) /
      (cycles * cycleTime);
  avg_cache_leak = cache_leakagePower;
  avg_cache_pwr = avg_cache_ac_pwr + avg_cache_leak;

  load_queue.getAveragePower(
      cycles, cycleTime, &avg_lq_pwr, &avg_lq_ac_pwr, &avg_lq_leak);
  store_queue.getAveragePower(
      cycles, cycleTime, &avg_sq_pwr, &avg_sq_ac_pwr, &avg_sq_leak);
  dtb.getAveragePower(
      cycles, cycleTime, &avg_tlb_pwr, &avg_tlb_ac_pwr, &avg_tlb_leak);

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
  power_file << "system.datapath.dcache.area " << cache_area
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

void
HybridDatapath::getAverageSpadPower(
    unsigned int cycles, float *avg_power, float *avg_dynamic, float *avg_leak)
{
  scratchpad->getAveragePower(cycles, avg_power, avg_dynamic, avg_leak);
}

double
HybridDatapath::getTotalMemArea()
{
  return scratchpad->getTotalArea() + cache_area;
}

void HybridDatapath::getMemoryBlocks(std::vector<std::string>& names) {}

void HybridDatapath::getRegisterBlocks(std::vector<std::string>& names) {}

Addr
HybridDatapath::getBaseAddress(std::string label)
{
  return (Addr) BaseDatapath::getBaseAddress(label);
}

////////////////////////////////////////////////////////////////////////////
//
//  The SimObjects we use to get the Datapath information into the simulator
//
////////////////////////////////////////////////////////////////////////////

HybridDatapath*
HybridDatapathParams::create()
{
  return new HybridDatapath(this);
}
