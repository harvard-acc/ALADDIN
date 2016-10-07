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
#include "aladdin_sys_constants.h"
#include "aladdin_tlb.hh"
#include "HybridDatapath.h"

const unsigned int maxInflightNodes = 100;

HybridDatapath::HybridDatapath(const HybridDatapathParams* params)
    : ScratchpadDatapath(
          params->benchName, params->traceFileName, params->configFileName),
      Gem5Datapath(params,
                   params->acceleratorId,
                   params->executeStandalone,
                   params->system),
      dmaSetupLatency(params->dmaSetupLatency),
      spadPort(this,
               params->system,
               params->maxDmaRequests,
               params->dmaChunkSize,
               params->multiChannelDMA),
      spadMasterId(params->system->getMasterId(name() + ".spad")),
      cachePort(this),
      cacheMasterId(params->system->getMasterId(name() + ".cache")),
      memory_queue(),
      enable_stats_dump(params->enableStatsDump), cacheSize(params->cacheSize),
      cacti_cfg(params->cactiCacheConfig), cacheLineSize(params->cacheLineSize),
      cacheHitLatency(params->cacheHitLatency), cacheAssoc(params->cacheAssoc),
      cache_readEnergy(0), cache_writeEnergy(0), cache_leakagePower(0),
      cache_area(0), dtb(this,
                         params->tlbEntries,
                         params->tlbAssoc,
                         params->tlbHitLatency,
                         params->tlbMissLatency,
                         params->tlbPageBytes,
                         params->numOutStandingWalks,
                         params->tlbBandwidth,
                         params->tlbCactiConfig,
                         params->acceleratorName),
      issueDmaOpsASAP(params->issueDmaOpsASAP),
      ignoreCacheFlush(params->ignoreCacheFlush),
      tickEvent(this), delayedDmaEvent(this), executedNodesLastTrigger(0) {
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
  isCacheBlocked = false;
  retryPkt = nullptr;

  /* In standalone mode, we charge the constant cost of a DMA setup operation
   * up front, instead of per dmaLoad call, or we'll overestimate the setup
   * latency.
   */
  if (execute_standalone) {
    initializeDatapath(dmaSetupLatency);
  }

  /* For the DMA model, compute the cost of a CPU cache flush in terms of
   * accelerator cycles.  Yeah, this is backwards -- we should be doing this
   * from the CPU itself -- but in standalone mode where we don't have a CPU,
   * this is the next best option.
   *
   * Latencies were characterized from Zynq Zedboard running at 666MHz.
   */
  cacheLineFlushLatency = ceil((56.0 * 1.5) / BaseDatapath::cycleTime);
  cacheLineInvalidateLatency = ceil((47.0 * 1.5) / BaseDatapath::cycleTime);
}

HybridDatapath::~HybridDatapath() {
  if (!execute_standalone)
    system->deregisterAccelerator(accelerator_id);
}

void HybridDatapath::clearDatapath(bool flush_tlb) {
  ScratchpadDatapath::clearDatapath();
  dtb.resetCounters();
  resetCounters(flush_tlb);
}

void HybridDatapath::resetCounters(bool flush_tlb) {
  loads = 0;
  stores = 0;
  dma_setup_cycles = 0;
  if (flush_tlb)
    dtb.clear();
}

void HybridDatapath::clearDatapath() { clearDatapath(false); }

void HybridDatapath::initializeDatapath(int delay) {
  buildDddg();
  globalOptimizationPass();
  prepareForScheduling();
  num_cycles = delay;
  isCacheBlocked = false;
  retryPkt = NULL;
  startDatapathScheduling(delay);
  if (ready_mode)
    scratchpad->resetReadyBits();
}

void HybridDatapath::startDatapathScheduling(int delay) {
  scheduleOnEventQueue(delay);
}

BaseMasterPort& HybridDatapath::getMasterPort(const std::string& if_name,
                                              PortID idx) {
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

void HybridDatapath::insertTLBEntry(Addr vaddr, Addr paddr) {
  DPRINTF(HybridDatapath, "Mapping vaddr 0x%x -> paddr 0x%x.\n", vaddr, paddr);
  Addr vpn = vaddr & ~(dtb.pageMask());
  Addr ppn = paddr & ~(dtb.pageMask());
  DPRINTF(
      HybridDatapath, "Inserting TLB entry vpn 0x%x -> ppn 0x%x.\n", vpn, ppn);
  dtb.insertBackupTLB(vpn, ppn);
}

void HybridDatapath::insertArrayLabelToVirtual(std::string array_label,
                                               Addr vaddr) {
  DPRINTF(HybridDatapath,
          "Inserting array label mapping %s -> vpn 0x%x.\n",
          array_label.c_str(),
          vaddr);
  dtb.insertArrayLabelToVirtual(array_label, vaddr);
}

void HybridDatapath::eventStep() {
  step();
  scratchpad->step();
}

void HybridDatapath::delayedDmaIssue() {
  // Pop off the front entry, which is the one that this event is being
  // triggered for, and push it onto the issue queue.
  auto waiting_entry = dmaWaitingQueue.front();
  dmaWaitingQueue.pop_front();
  unsigned node_id = waiting_entry.first;
  addDmaNodeToIssueQueue(node_id);

  // "DMA setup latency" in this model means the time required to flush ALL the
  // cache lines being transmitted and invalidate ALL the cache lines for the
  // receiving buffer. Typically, DMA transfer only starts after this process
  // is completely finished. This optimization lets us issue DMA nodes as soon
  // as they have finished their own setup, rather than waiting for everyone's
  // setup to finish.
  if (issueDmaOpsASAP) {
    issueDmaRequest(node_id);
  }

  // Schedule the next delayed DMA event.
  if (!dmaWaitingQueue.empty() && !delayedDmaEvent.scheduled()) {
    auto next_entry = dmaWaitingQueue.front();
    node_id = next_entry.first;
    Tick delay = next_entry.second;
    schedule(delayedDmaEvent, delay + curTick());
    DPRINTF(HybridDatapath,
            "Next setup latency for DMA operation with id %d: %d ticks.\n",
            node_id, delay);
  }

  // In the typical case, after all DMA nodes have exited the waiting queue, we
  // can issue them all at once. No need to wait anymore.
  if (!issueDmaOpsASAP && dmaWaitingQueue.empty()) {
    for (auto it = dmaIssueQueue.begin(); it != dmaIssueQueue.end(); it++)
      issueDmaRequest(it->second);
  }
}

void HybridDatapath::stepExecutingQueue() {
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
      if (inflight_multicycle_nodes.find(node_id) ==
          inflight_multicycle_nodes.end()) {
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

void HybridDatapath::retireReturnedMemQEntries() {
  memory_queue.retireReturnedEntries();
}

void HybridDatapath::markNodeStarted(ExecNode* node) {
  BaseDatapath::markNodeStarted(node);
}

bool HybridDatapath::step() {
  executedNodesLastTrigger = executedNodes;
  resetCacheCounters();
  stepExecutingQueue();
  copyToExecutingQueue();
  retireReturnedMemQEntries();
  DPRINTF(HybridDatapath,
          "[CYCLES]:%d. executedNodes:%d, totalConnectedNodes:%d\n",
          num_cycles,
          executedNodes,
          totalConnectedNodes);
  if (executedNodesLastTrigger == executedNodes)
    cycles_since_last_node++;
  else
    cycles_since_last_node = 0;
  if (cycles_since_last_node > deadlock_threshold) {
    exitSimLoop("Deadlock detected!");
    return false;
  }

  num_cycles++;
  if (executedNodes < totalConnectedNodes) {
    schedule(tickEvent, clockEdge(Cycles(1)));
    return true;
  } else {
    dumpStats();
    DPRINTF(Aladdin, "Accelerator completed.\n");
    if (execute_standalone) {
      // If in standalone mode, we wait dmaSetupLatency before the datapath
      // starts, but this cost must be added to this stat. For some reason,
      // this cannot be done until we have started simulation of events,
      // because all stats get cleared before that happens.  So until I figure
      // out how to get around that, it's going to be accounted for at the end.
      dma_setup_cycles += dmaSetupLatency;
      system->deregisterAccelerator(accelerator_id);
      if (system->numRunningAccelerators() == 0) {
        exitSimLoop("Aladdin called exit()");
      }
    } else {
      if (enable_stats_dump) {
        std::string exit_reason =
            DUMP_STATS_EXIT_SIM_SIGNAL + datapath_name + " completed.";
        exitSimLoop(exit_reason);
      }
      sendFinishedSignal();
      clearDatapath(false);
    }
  }
  return false;
}

HybridDatapath::MemoryOpType HybridDatapath::getMemoryOpType(ExecNode* node) {
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

bool HybridDatapath::handleRegisterMemoryOp(ExecNode* node) {
  markNodeStarted(node);
  std::string reg = node->get_array_label();
  if (node->is_load_op())
    registers.getRegister(reg)->increment_loads();
  else
    registers.getRegister(reg)->increment_stores();
  return true;
}

bool HybridDatapath::handleSpadMemoryOp(ExecNode* node) {
  std::string array_name = node->get_array_label();
  unsigned array_name_index = node->get_partition_index();
  MemAccess* mem_access = node->get_mem_access();
  uint64_t vaddr = mem_access->vaddr;
  bool isLoad = node->is_load_op();
  bool satisfied = scratchpad->canServicePartition(
                       array_name, array_name_index, vaddr, isLoad);
  if (!satisfied)
    return false;

  markNodeStarted(node);
  if (isLoad) {
    scratchpad->increment_loads(array_name, array_name_index);
  } else {
    size_t size = mem_access->size;
    uint8_t* data = new uint8_t[size];
    memcpy(data, &(mem_access->value), size);
    scratchpad->writeData(array_name, vaddr, data, size);
    scratchpad->increment_stores(array_name, array_name_index);
  }
  return true;
}

bool HybridDatapath::handleDmaMemoryOp(ExecNode* node) {
  MemAccessStatus status = Ready;
  unsigned node_id = node->get_node_id();
  if (inflight_dma_nodes.find(node_id) == inflight_dma_nodes.end())
    inflight_dma_nodes[node_id] = status;
  else
    status = inflight_dma_nodes[node_id];
  if (status == Ready && inflight_dma_nodes.size() < maxInflightNodes) {
    /* A DMA load needs to be preceded by a cache flush of the buffer being sent,
     * while a DMA store needs to be preceded by a cache invalidate of the
     * receiving buffer. In CPU mode, this should be handled by the CPU; in standalone
     * mode, we add extra latency to the DMA operation here.
     */
    size_t size = node->get_mem_access()->size;
    bool isLoad = node->is_dma_load();
    unsigned cache_delay_cycles = 0;
    unsigned cache_lines_affected =
        ceil(((float)size) / system->cacheLineSize());
    if (!ignoreCacheFlush) {
      if (isLoad)
        cache_delay_cycles = cache_lines_affected * cacheLineFlushLatency;
      else
        cache_delay_cycles = cache_lines_affected * cacheLineInvalidateLatency;
    }
    dma_setup_cycles += cache_delay_cycles;
    Tick delay = clockPeriod() * cache_delay_cycles;
    if (!delayedDmaEvent.scheduled()) {
      DPRINTF(HybridDatapath,
              "Scheduling DMA %s operation with node id %d with delay %lu\n",
              isLoad ? "load" : "store", node_id, delay);
      schedule(delayedDmaEvent, curTick() + delay);
    }
    inflight_dma_nodes[node_id] = WaitingForDmaSetup;
    dmaWaitingQueue.push_back(std::make_pair(node_id, delay));
    return false;  // DMA op not completed. Move on to the next node.
  } else if (status == Returned) {
    std::string array_label = node->get_array_label();
    if (node->is_dma_load() && ready_mode)
      // TODO: Will be replaced by call backs from dma_device.
      scratchpad->setReadyBits(array_label);
    inflight_dma_nodes.erase(node_id);
    return true;  // DMA op completed.
  } else {
    // Still waiting from the cache or for DMA setup to finish. Move on to the next node.
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

  if (memory_queue.contains(vaddr))
    inflight_mem_op = memory_queue.getStatus(vaddr);

  Stats::Scalar& mem_stat = isLoad ? loads : stores;
  if (inflight_mem_op == Ready) {
    // This is either the first time we're seeing this node, or we are retrying
    // this node after a TLB miss. We'll enqueue it - if it already exists in
    // the memory_queue, a duplicate won't be added.
    if (memory_queue.insert(vaddr))
      mem_stat++;

    int size = mem_access->size;
    if (dtb.canRequestTranslation() &&
        issueTLBRequestTiming(vaddr, size, isLoad, node_id)) {
      markNodeStarted(node);
      dtb.incrementRequestCounter();
      memory_queue.setStatus(vaddr, Translating);
      DPRINTF(HybridDatapath,
              "node:%d, vaddr = %x, is translating\n",
              node_id,
              vaddr);
    }
    return false;
  } else if (inflight_mem_op == Translated) {
    Addr paddr = mem_access->paddr;
    int size = mem_access->size;
    uint64_t value = mem_access->value;

    if (issueCacheRequest(paddr, size, isLoad, node_id, value)) {
      memory_queue.setStatus(vaddr, WaitingFromCache);
      DPRINTF(HybridDatapath,
              "node:%d, vaddr = 0x%x, paddr = 0x%x is accessing cache\n",
              node_id,
              vaddr,
              paddr);
    }
    return false;
  } else if (inflight_mem_op == Returned) {
    // Memory ops that have returned will be freed at the end of the cycle.
    return true;
  } else {
    return false;
  }
}

// Issue a DMA request for memory.
void HybridDatapath::issueDmaRequest(unsigned node_id) {
  ExecNode *node = exec_nodes[node_id];
  markNodeStarted(node);
  bool isLoad = node->is_dma_load();
  MemAccess* mem_access = node->get_mem_access();
  Addr base_addr = mem_access->vaddr;
  size_t offset = mem_access->offset;
  size_t size = mem_access->size;  // In bytes.
  DPRINTF(
      HybridDatapath, "issueDmaRequest for addr:%#x, size:%u\n", base_addr+offset, size);
  /* Assigning the array label can (and probably should) be done in the
   * optimization pass instead of the scheduling pass. */
  auto part_it = getArrayConfigFromAddr(base_addr);
  MemoryType mtype = part_it->second.memory_type;
  assert(mtype == spad || mtype == reg);
  std::string array_label = part_it->first;
  node->set_array_label(array_label);
  if (mtype == spad)
    incrementDmaScratchpadAccesses(array_label, size, isLoad);
  else
    registers.getRegister(array_label)->increment_dma_accesses(isLoad);
  inflight_dma_nodes[node_id] = WaitingFromDma;
  Addr vaddr = (base_addr + offset) & ADDR_MASK;
  // If this is a dmaStore, we issue a WriteInvalidateReq in order to
  // invalidate any shared cache lines in CPU caches.
  MemCmd::Command cmd = isLoad ? MemCmd::ReadReq : MemCmd::WriteInvalidateReq;
  Request::Flags flag = 0;

  /* In order to make DMA stores visible to the rest of the memory hierarchy,
   * we have to do a virtual address translation. However, we do this
   * "invisibly" in zero time.  This is because DMAs are typically initiated by
   * the CPU using physical addresses, but our DMA model initiates transfers
   * from the accelerator, which can't know physical addresses without a
   * translation.
   */
  issueTLBRequestInvisibly(vaddr, size, isLoad, node_id);
  Addr paddr = mem_access->paddr;
  uint8_t* data = new uint8_t[size];
  if (!isLoad) {
    scratchpad->readData(array_label, vaddr, size, data);
  }

  DmaEvent* dma_event = new DmaEvent(this, node_id);
  spadPort.dmaAction(
      cmd, paddr, size, dma_event, data, 0, flag);
}

/* Mark the DMA request node as having completed. */
void HybridDatapath::completeDmaRequest(unsigned node_id) {
  MemAccess* mem_access = exec_nodes[node_id]->get_mem_access();
  Addr vaddr = (mem_access->vaddr + mem_access->offset) & ADDR_MASK;
  dmaIssueQueue.erase(vaddr);
  DPRINTF(HybridDatapath,
          "completeDmaRequest for addr:%#x \n",
          vaddr);
  inflight_dma_nodes[node_id] = Returned;
}

void HybridDatapath::addDmaNodeToIssueQueue(unsigned node_id) {
  DPRINTF(HybridDatapath, "Adding DMA node %d to DMA issue queue.\n", node_id);
  MemAccess* mem_access = exec_nodes[node_id]->get_mem_access();
  Addr vaddr = (mem_access->vaddr + mem_access->offset) & ADDR_MASK;
  dmaIssueQueue[vaddr] = node_id;
}

void HybridDatapath::sendFinishedSignal() {
  Flags<Packet::FlagsType> flags = 0;
  size_t size = 4;  // 32 bit integer.
  uint8_t* data = new uint8_t[size];
  // Set some sentinel value.
  for (int i = 0; i < size; i++)
    data[i] = 0x13;
  Request* req = new Request(finish_flag, size, flags, getCacheMasterId());
  req->setThreadContext(context_id, thread_id);  // Only needed for prefetching.
  MemCmd::Command cmd = MemCmd::WriteReq;
  PacketPtr pkt = new Packet(req, cmd);
  pkt->dataStatic<uint8_t>(data);
  DatapathSenderState* state = new DatapathSenderState(-1, true);
  pkt->senderState = state;

  if (!cachePort.sendTimingReq(pkt)) {
    assert(retryPkt == NULL);
    retryPkt = pkt;
    isCacheBlocked = true;
    DPRINTF(HybridDatapath, "Sending finished signal failed, retrying.\n");
  } else {
    DPRINTF(HybridDatapath, "Sent finished signal.\n");
  }
}

void HybridDatapath::CachePort::recvReqRetry() {
  DPRINTF(HybridDatapath, "recvReqRetry for addr:");
  assert(datapath->isCacheBlocked && datapath->retryPkt != NULL);
  DPRINTF(HybridDatapath, "%#x \n", datapath->retryPkt->getAddr());
  if (datapath->cachePort.sendTimingReq(datapath->retryPkt)) {
    DPRINTF(HybridDatapath, "Retry pass!\n");
    datapath->retryPkt = NULL;
    datapath->isCacheBlocked = false;
  } else
    DPRINTF(HybridDatapath, "Still blocked!\n");
}

bool HybridDatapath::CachePort::recvTimingResp(PacketPtr pkt) {
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

bool HybridDatapath::issueTLBRequestTiming(
    Addr addr, unsigned size, bool isLoad, unsigned node_id) {
  DPRINTF(HybridDatapath, "issueTLBRequestTiming for trace addr:%#x\n", addr);
  PacketPtr data_pkt = createTLBRequestPacket(addr, size, isLoad, node_id);

  // TLB access
  if (dtb.translateTiming(data_pkt))
    return true;
  else {
    delete data_pkt->senderState;
    delete data_pkt;
    return false;
  }
}

bool HybridDatapath::issueTLBRequestInvisibly(
    Addr addr, unsigned size, bool isLoad, unsigned node_id) {
  PacketPtr data_pkt = createTLBRequestPacket(addr, size, isLoad, node_id);
  dtb.translateInvisibly(data_pkt);
  // data_pkt now contains the translated address.
  ExecNode* node = exec_nodes[node_id];
  node->get_mem_access()->paddr = *data_pkt->getPtr<Addr>();
  return true;
}

PacketPtr HybridDatapath::createTLBRequestPacket(
    Addr addr, unsigned size, bool isLoad, unsigned node_id) {
  Request* req = NULL;
  Flags<Packet::FlagsType> flags = 0;
  // Constructor for physical request only
  req = new Request(addr, size, flags, getCacheMasterId());

  MemCmd command;
  if (isLoad)
    command = MemCmd::ReadReq;
  else
    command = MemCmd::WriteReq;
  PacketPtr data_pkt = new Packet(req, command);

  Addr* translation = new Addr;
  *translation = 0x0;  // Signifies no translation.
  data_pkt->dataStatic<Addr>(translation);

  AladdinTLB::TLBSenderState* state = new AladdinTLB::TLBSenderState(node_id);
  data_pkt->senderState = state;
  return data_pkt;
}

void HybridDatapath::completeTLBRequest(PacketPtr pkt, bool was_miss) {
  AladdinTLB::TLBSenderState* state =
      dynamic_cast<AladdinTLB::TLBSenderState*>(pkt->senderState);
  DPRINTF(HybridDatapath,
          "completeTLBRequest for addr:%#x %s\n",
          pkt->getAddr(),
          pkt->cmdString());
  Addr vaddr = pkt->getAddr();
  unsigned node_id = state->node_id;
  ExecNode* node = exec_nodes[node_id];
  assert(memory_queue.contains(vaddr) && memory_queue.getStatus(vaddr) == Translating);

  /* If the TLB missed, we need to retry the complete instruction, as the dcache
   * state may have changed during the waiting time, so we go back to Ready
   * instead of Translated. The next translation request should hit.
   */
  if (was_miss) {
    memory_queue.setStatus(vaddr, Ready);
  } else {
    // Translations are actually the complete memory address, not just the page
    // number, because of the trace to virtual address translation.
    Addr paddr = *pkt->getPtr<Addr>();
    MemAccess* mem_access = node->get_mem_access();
    mem_access->paddr = paddr;
    memory_queue.setStatus(vaddr, Translated);
    memory_queue.setPhysicalAddress(vaddr, paddr);
    DPRINTF(HybridDatapath,
            "node:%d was translated, vaddr = %x, paddr = %x\n",
            node_id,
            vaddr,
            paddr);
  }
  delete state;
  delete pkt->req;
  delete pkt;
}

bool HybridDatapath::issueCacheRequest(Addr addr,
                                       unsigned size,
                                       bool isLoad,
                                       unsigned node_id,
                                       uint64_t value) {
  DPRINTF(HybridDatapath, "issueCacheRequest for addr:%#x\n", addr);
  if (isCacheBlocked) {
    DPRINTF(HybridDatapath, "MSHR is full. Waiting for misses to return.\n");
    return false;
  }

  Request* req = NULL;
  Flags<Packet::FlagsType> flags = 0;
  /* To use strided prefetching, we need to include a "PC" so the prefetcher
   * can predict memory behavior similar to how branch predictors work. We
   * don't have real PCs in aladdin so we'll just hash the unique id of the
   * node.  */
  std::string unique_id = exec_nodes[node_id]->get_static_node_id();
  Addr pc = static_cast<Addr>(std::hash<std::string>()(unique_id));
  req = new Request(addr, size, flags, getCacheMasterId(), curTick(), pc);
  /* The context id and thread ids are needed to pass a few assert checks in
   * gem5, but they aren't actually required for the mechanics of the memory
   * checking itsef. This has to be set outside of the constructor or the
   * address will be interpreted as a virtual, not physical. */
  req->setThreadContext(context_id, thread_id);

  MemCmd command;
  uint8_t* data = new uint8_t[size];
  memcpy(data, &value, size);
  if (isLoad) {
    command = MemCmd::ReadReq;
  } else {
    command = MemCmd::WriteReq;
  }
  PacketPtr data_pkt = new Packet(req, command);
  data_pkt->dataStatic(data);

  DatapathSenderState* state = new DatapathSenderState(node_id);
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
    else {
      DPRINTF(HybridDatapath,
              "Node id %d store of value %lf to address %#x "
              "issued to dcache!\n",
              node_id,
              value,
              addr);
    }
  }

  return true;
}

/* Data that is returned gets written back into the memory queue.
 * Note that due to our trace driven execution framework, we don't actually
 * care about the dynamic data returned by a load. We only care that a store
 * successfully writes the value into the cache.
 */
void HybridDatapath::completeCacheRequest(PacketPtr pkt) {
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

  bool isLoad = (pkt->cmd == MemCmd::ReadResp);
  MemAccess* mem_access = exec_nodes[node_id]->get_mem_access();
  Addr vaddr = mem_access->vaddr;
  if (isExecuteStandalone())
    vaddr &= ADDR_MASK;
  memory_queue.setStatus(vaddr, Returned);
  DPRINTF(HybridDatapath,
          "setting %s memory_queue entry for address %#x to Returned.\n",
          isLoad ? "load" : "store",
          vaddr);

  delete state;
  delete pkt->req;
  delete pkt;
}

/* Receiving response from DMA. */
bool HybridDatapath::SpadPort::recvTimingResp(PacketPtr pkt) {
  // Get the DMA sender state to retrieve node id, so we can get the virtual
  // address (the packet address is possibly physical).
  DmaEvent* event = dynamic_cast<DmaEvent*>(getPacketCompletionEvent(pkt));
  assert(event != nullptr && "Packet completion event is not a DmaEvent object!");

  unsigned node_id = event->get_node_id();
  ExecNode * node = datapath->getNodeFromNodeId(node_id);
  assert(node != nullptr && "DMA node id is invalid!");

  MemAccess* mem_access = node->get_mem_access();
  Addr vaddr_base = (mem_access->vaddr + mem_access->offset) & ADDR_MASK;
  assert(datapath->dmaIssueQueue[vaddr_base] == node_id &&
         "Node id in packet and issue queue do not match!");

  // This will compute the offset of THIS packet from the base address.
  Addr paddr = pkt->getAddr();
  Addr paddr_base = getPacketAddr(pkt);
  Addr pkt_offset =  paddr - paddr_base;
  Addr vaddr = vaddr_base + pkt_offset;

  std::string array_label = node->get_array_label();
  unsigned size = pkt->req->getSize(); // in bytes
  DPRINTF(HybridDatapath,
          "Receiving DMA response for node %d, address %#x with label %s and size %d.\n",
          node_id, vaddr, array_label.c_str(), size);
  if (datapath->ready_mode)
    datapath->scratchpad->setReadyBitRange(array_label, vaddr, size);

  // For dmaLoads, write the data into the scratchpad.
  if (node->is_dma_load()) {
    uint8_t* data = pkt->getPtr<uint8_t>();
    assert(data != nullptr && "Packet data is null!");

    datapath->scratchpad->writeData(array_label, vaddr, data, size);
  }
  return DmaPort::recvTimingResp(pkt);
}

HybridDatapath::DmaEvent::DmaEvent(HybridDatapath* _dpath, unsigned _dma_node_id)
    : Event(Default_Pri, AutoDelete), datapath(_dpath), dma_node_id(_dma_node_id) {}

void HybridDatapath::DmaEvent::process() {
  assert(!datapath->dmaIssueQueue.empty());
  datapath->completeDmaRequest(dma_node_id);
}

const char* HybridDatapath::DmaEvent::description() const {
  return "Hybrid DMA datapath receiving request event";
}

void HybridDatapath::resetCacheCounters() {
  dtb.resetRequestCounter();
}

void HybridDatapath::regStats() {
  using namespace Stats;
  DPRINTF(HybridDatapath, "Registering stats.\n");
  loads.name("system." + datapath_name + ".total_loads")
      .desc("Total number of dcache loads")
      .flags(total | nonan);
  stores.name("system." + datapath_name + ".total_stores")
      .desc("Total number of dcache stores.")
      .flags(total | nonan);
  dma_setup_cycles.name("system." + datapath_name + ".dma_setup_cycles")
      .desc("Total number of cycles spent on setting up DMA transfers.")
      .flags(total | nonan);
}

#ifdef USE_DB
int HybridDatapath::writeConfiguration(sql::Connection* con) {
  int unrolling_factor, partition_factor;
  bool pipelining;
  getCommonConfigParameters(unrolling_factor, pipelining, partition_factor);

  sql::Statement* stmt = con->createStatement();
  stringstream query;
  query << "insert into configs (id, memory_type, trace_file, "
           "config_file, pipelining, unrolling, partitioning, "
           "max_dma_requests, dma_setup_latency, cache_size, cache_line_sz, "
           "cache_assoc, cache_hit_latency, "
           "tlb_page_size, tlb_assoc, tlb_miss_latency, "
           "tlb_hit_latency, tlb_max_outstanding_walks, tlb_bandwidth, "
           "tlb_entries) values (";
  query << "NULL"
        << ",\"spad\""
        << ","
        << "\"" << trace_file << "\""
        << ",\"" << config_file << "\"," << pipelining << ","
        << unrolling_factor << "," << partition_factor << ","
        << spadPort.max_req << "," << dmaSetupLatency << ",\"" << cacheSize
        << "\"," << cacheLineSize << "," << cacheAssoc << "," << cacheHitLatency
        << "," << dtb.getPageBytes() << ","
        << dtb.getAssoc() << "," << dtb.getMissLatency() << ","
        << dtb.getHitLatency() << "," << dtb.getNumOutStandingWalks() << ","
        << dtb.bandwidth << "," << dtb.getNumEntries() << ")";
  stmt->execute(query.str());
  delete stmt;
  // Get the newly added config_id.
  return getLastInsertId(con);
}
#endif

void HybridDatapath::computeCactiResults() {
  // If no caches wer used, then we don't need to invoke CACTI.
  if (cacti_cfg.empty())
    return;
  DPRINTF(
      HybridDatapath, "Invoking CACTI for cache power and area estimates.\n");
  uca_org_t cacti_result = cacti_interface(cacti_cfg);
  // power in mW, energy in pJ, area in mm2
  cache_readEnergy = cacti_result.power.readOp.dynamic * 1e12;
  cache_writeEnergy = cacti_result.power.writeOp.dynamic * 1e12;
  cache_leakagePower = cacti_result.power.readOp.leakage * 1000;
  cache_area = cacti_result.area;

  dtb.computeCactiResults();
  cacti_result.cleanup();
}

void HybridDatapath::getAverageMemPower(unsigned int cycles,
                                        float* avg_power,
                                        float* avg_dynamic,
                                        float* avg_leak) {
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

void HybridDatapath::getAverageCachePower(unsigned int cycles,
                                          float* avg_power,
                                          float* avg_dynamic,
                                          float* avg_leak) {
  float avg_cache_pwr, avg_cache_leak, avg_cache_ac_pwr;
  float avg_tlb_pwr, avg_tlb_leak, avg_tlb_ac_pwr;

  avg_cache_ac_pwr =
      (loads.value() * cache_readEnergy + stores.value() * cache_writeEnergy) /
      (cycles * cycleTime);
  avg_cache_leak = cache_leakagePower;
  avg_cache_pwr = avg_cache_ac_pwr + avg_cache_leak;

  dtb.getAveragePower(
      cycles, cycleTime, &avg_tlb_pwr, &avg_tlb_ac_pwr, &avg_tlb_leak);

  *avg_dynamic = avg_cache_ac_pwr;
  *avg_leak = avg_cache_leak;
  *avg_power = avg_cache_pwr;

  // TODO: Write this detailed data to the database as well.
  std::string power_file_name = benchName + "_power_stats.txt";
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
  power_file << "system.datapath.tlb.area " << dtb.getArea() << " # tlb area."
             << std::endl;

  power_file.close();
}

void HybridDatapath::getAverageSpadPower(unsigned int cycles,
                                         float* avg_power,
                                         float* avg_dynamic,
                                         float* avg_leak) {
  scratchpad->getAveragePower(cycles, avg_power, avg_dynamic, avg_leak);
}

double HybridDatapath::getTotalMemArea() {
  return scratchpad->getTotalArea() + cache_area;
}

void HybridDatapath::getMemoryBlocks(std::vector<std::string>& names) {}

void HybridDatapath::getRegisterBlocks(std::vector<std::string>& names) {}

Addr HybridDatapath::getBaseAddress(std::string label) {
  return (Addr)BaseDatapath::getBaseAddress(label);
}

// Increment the scratchpad load/store counters based on DMA transfer size.
void HybridDatapath::incrementDmaScratchpadAccesses(std::string array_label,
                                                    unsigned dma_size,
                                                    bool is_dma_load) {
  DPRINTF(
      HybridDatapath, "DMA Accesses: array label %s\n", array_label.c_str());
  if (is_dma_load) {
    scratchpad->increment_dma_loads(array_label, dma_size);
    DPRINTF(HybridDatapath,
            "Increment the dmaLoad accesses of partition %s with size %d. \n",
            array_label,
            dma_size);
  } else {
    scratchpad->increment_dma_stores(array_label, dma_size);
    DPRINTF(HybridDatapath,
            "Increment the dmaStore accesses of partition %s with size %d. \n",
            array_label,
            dma_size);
  }
}

////////////////////////////////////////////////////////////////////////////
//
//  The SimObjects we use to get the Datapath information into the simulator
//
////////////////////////////////////////////////////////////////////////////

HybridDatapath* HybridDatapathParams::create() {
  return new HybridDatapath(this);
}
