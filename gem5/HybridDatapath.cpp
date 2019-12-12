// Implementation of a hybrid datapath with both caches and scratchpads.

#include <cstdint>
#include <functional>  // For std::hash
#include <sstream>
#include <string>

#include "base/flags.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "base/logging.hh"
#include "base/chunk_generator.hh"
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
#include "aladdin/common/DynamicEntity.h"
#include "debug/Aladdin.hh"
#include "debug/HybridDatapath.hh"
#include "debug/HybridDatapathVerbose.hh"
#include "aladdin_sys_constants.h"
#include "aladdin_tlb.hh"
#include "HybridDatapath.h"

const unsigned int maxInflightNodes = 100;

HybridDatapath::HybridDatapath(const HybridDatapathParams* params)
    : ScratchpadDatapath(
          params->outputPrefix, params->traceFileName, params->configFileName),
      Gem5Datapath(params,
                   params->acceleratorId,
                   params->maxDmaRequests,
                   params->dmaChunkSize,
                   params->numDmaChannels,
                   params->invalidateOnDmaStore,
                   params->system),
      cache_queue(params->cacheQueueSize,
                  params->cacheBandwidth,
                  system->cacheLineSize(),
                  params->cactiCacheQueueConfig),
      busy(false), ackedFinishSignal(false),
      enable_stats_dump(params->enableStatsDump),
      acp_enabled(params->enableAcp), use_acp_cache(params->useAcpCache),
      cacheSize(params->cacheSize), cacti_cfg(params->cactiCacheConfig),
      cacheLineSize(system->cacheLineSize()),
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
      pipelinedDma(params->pipelinedDma),
      ignoreCacheFlush(params->ignoreCacheFlush), tickEvent(this),
      use_aladdin_debugger(params->useAladdinDebugger), delayedDmaEvent(this),
      enterDebuggerEvent(this), executedNodesLastTrigger(0) {
  BaseDatapath::use_db = params->useDb;
  BaseDatapath::experiment_name = params->experimentName;
#ifdef USE_DB
  if (use_db) {
    BaseDatapath::setExperimentParameters(experiment_name);
  }
#endif
  float cycle_time = params->cycleTime;
  BaseDatapath::user_params.cycle_time = cycle_time;
  datapath_name = params->acceleratorName;
  cache_queue.initStats(datapath_name + ".cache_queue");
  system->registerAccelerator(accelerator_id, this);

  /* For the DMA model, compute the cost of a CPU cache flush in terms of
   * accelerator cycles.  Yeah, this is backwards -- we should be doing this
   * from the CPU itself.
   */
  cacheLineFlushLatency = params->cacheLineFlushLatency / cycle_time;
  cacheLineInvalidateLatency = params->cacheLineFlushLatency / cycle_time;

  if (use_aladdin_debugger)
    adb::init_debugger();
}

HybridDatapath::~HybridDatapath() {
  system->deregisterAccelerator(accelerator_id);
}

void HybridDatapath::clearDatapath() {
  ScratchpadDatapath::clearDatapath();
  dtb.clear();
}

void HybridDatapath::resetCounters() {
  dcache_loads = 0;
  dcache_stores = 0;
  acp_loads = 0;
  acp_stores = 0;
  dma_setup_cycles = 0;
  dtb.resetCounters();
}

bool HybridDatapath::queueCommand(std::unique_ptr<AcceleratorCommand> cmd) {
  if (!busy) {
    // Directly run the command if the accelerator is not busy.
    cmd->run(this);
  } else {
    // Queue the command.
    DPRINTF(HybridDatapath, "Queuing command %s on accelerator %d.\n",
            cmd->name(), accelerator_id);
    commandQueue.push_back(std::move(cmd));
  }
  return true;
}

void HybridDatapath::initializeDatapath(int delay) {
  DPRINTF(HybridDatapath, "Activating accelerator id %d\n", accelerator_id);
  gettimeofday(&beginTime, NULL);
  busy = true;
  // Don't reset stats - the user can reset them manually through the CPU
  // interface or by setting the --enable-stats-dump flag.
  ScratchpadDatapath::clearDatapath();
  bool dddg_built = buildDddg();
  if (!dddg_built) {
    exitSimulation();
    return;
  }
  enterDebuggerIfEnabled();

  globalOptimizationPass();
  prepareForScheduling();
  enterDebuggerIfEnabled();
  adb::execution_status = adb::SCHEDULING;

  num_cycles += delay;
  acc_sim_cycles += delay;
  cachePort.clearRetryPkt();
  startDatapathScheduling(delay);
  if (isReadyMode()) {
    // Ready bits must be explicitly set by the user.
    scratchpad->setReadyBits();
  }
}

void HybridDatapath::startDatapathScheduling(int delay) {
  scheduleOnEventQueue(delay);
}

void HybridDatapath::insertTLBEntry(Addr vaddr, Addr paddr) {
  DPRINTF(HybridDatapath, "Mapping vaddr 0x%x -> paddr 0x%x.\n", vaddr, paddr);
  Addr vpn = vaddr & ~(dtb.pageMask());
  Addr ppn = paddr & ~(dtb.pageMask());
  DPRINTF(
      HybridDatapath, "Inserting TLB entry vpn 0x%x -> ppn 0x%x.\n", vpn, ppn);
  dtb.insertBackupTLB(vpn, ppn);
}

void HybridDatapath::insertArrayLabelToVirtual(const std::string& array_label,
                                               Addr vaddr,
                                               size_t size) {
  DPRINTF(HybridDatapath,
          "Inserting array label mapping %s -> vpn 0x%x, size %d.\n",
          array_label.c_str(), vaddr, size);
  dtb.insertArrayLabelToVirtual(array_label, vaddr, size);
  try {
    user_params.updateArrayParams(array_label, size);
  } catch (UnknownArrayException& e) {
    fatal(e.what());
  }
  user_params.checkOverlappingRanges();
}

void HybridDatapath::setArrayMemoryType(const std::string& array_label,
                                        MemoryType mem_type) {
  assert((mem_type == dma || mem_type == acp || mem_type == cache) &&
         "The host memory access type can only be DMA, ACP or cache!");
  DPRINTF(HybridDatapath, "Setting %s to memory type %s.\n",
          array_label.c_str(), memoryTypeToString(mem_type));
  if (user_params.partition.find(array_label) != user_params.partition.end()) {
    // This array is found in the partitions.
    user_params.partition[array_label].memory_type = mem_type;
  } else {
    // New array. The dynamic trace will be parsed later to change other
    // attributes.
    user_params.partition[array_label] = { mem_type, none, 0, 0, 0, 0 };
  }
}

void HybridDatapath::resetTrace() {
  DPRINTF(HybridDatapath, "%s: Resetting dynamic trace\n.", name());
  BaseDatapath::resetTrace();
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
  if (pipelinedDma) {
    issueDmaRequest(node_id);
    dmaIssueQueue.erase(node_id);
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
  if (!pipelinedDma && dmaWaitingQueue.empty()) {
    for (auto it = dmaIssueQueue.begin(); it != dmaIssueQueue.end();) {
      issueDmaRequest(*it);
      dmaIssueQueue.erase(it++);
    }
  }
}

void HybridDatapath::printExecutingQueue() {
  std::cout << "Executing queue contents:\n";
  for (auto it = executingQueue.begin(); it != executingQueue.end(); it++)
    std::cout << (*it)->get_node_id() << ", ";
  std::cout << std::endl;
}

void HybridDatapath::stepExecutingQueue() {
  auto it = executingQueue.begin();
  int index = 0;
  while (it != executingQueue.end()) {
    ExecNode* node = *it;
    bool op_satisfied = false;
    if (node->is_memory_op() || node->is_host_mem_op()) {
      MemoryOpType type;
      try {
        type = getMemoryOpType(node);
      } catch (IllegalHostMemoryAccessException& e) {
        fatal(e.what());
      }
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
        case ReadyBits:
          op_satisfied = handleReadyBitAccess(node);
          break;
        case ACP:
          op_satisfied = handleAcpMemoryOp(node);
          break;
        default:
          panic("Unsupported memory operation type!");
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
        unsigned remaining_cycles = inflight_multicycle_nodes.at(node_id);
        if (remaining_cycles == 1) {
          inflight_multicycle_nodes.erase(node_id);
          markNodeCompleted(it, index);
          op_satisfied = true;
        } else {
          inflight_multicycle_nodes.at(node_id)--;
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
  cache_queue.retireReturnedEntries();
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
  DPRINTF(HybridDatapathVerbose,
          "[CYCLES]:%d. executedNodes:%d, totalConnectedNodes:%d\n",
          num_cycles,
          executedNodes,
          totalConnectedNodes);
  DPRINTF(HybridDatapathVerbose, "-----------------------------\n");
  if (executedNodesLastTrigger == executedNodes)
    cycles_since_last_node++;
  else
    cycles_since_last_node = 0;
  if (cycles_since_last_node > deadlock_threshold) {
    printExecutingQueue();
    exitSimLoop("Deadlock detected!");
    return false;
  }

  num_cycles++;
  acc_sim_cycles++;
  if (executedNodes < totalConnectedNodes) {
    schedule(tickEvent, clockEdge(Cycles(1)));
    return true;
  } else if (!upsampled) {
    dumpStats();
    schedule(tickEvent, clockEdge(Cycles(upsampled_cycles)));
  } else if (busy) {
    DPRINTF(Aladdin, "Accelerator completed.\n");
    timeval endTime;
    gettimeofday(&endTime, NULL);
    double elapsed = (endTime.tv_sec - beginTime.tv_sec) +
                     ((endTime.tv_usec - beginTime.tv_usec) / 1000000.0);
    DPRINTF(Aladdin, "Elapsed host seconds %.2f.\n", elapsed);
    busy = false;
    enterDebuggerIfEnabled();
    adb::execution_status = adb::POSTSCHEDULING;
    exitSimulation();
  } else if (ackedFinishSignal) {
    // Wake up the CPU thread after we receive the ack of the sent finish
    // signal. This ensures the CPU will see the new value of the finish flag
    // after it's woken up.
    wakeupCpuThread();
    DPRINTF(Aladdin, "Woken up the CPU thread context.\n");
    ackedFinishSignal = false;
    // If there are more commands, run them until we reach the next
    // ActivateAcceleratorCmd or the end of the queue.
    while (!commandQueue.empty()) {
      auto cmd = std::move(commandQueue.front());
      bool blocking = cmd->blocking();
      cmd->run(this);
      commandQueue.pop_front();
      if (blocking)
        break;
    }
  }
  return false;
}

void HybridDatapath::exitSimulation() {
  if (enable_stats_dump) {
    std::string exit_reason =
        DUMP_STATS_EXIT_SIM_SIGNAL + datapath_name + " completed.";
    exitSimLoop(exit_reason);
  }
  sendFinishedSignal();
  // Don't clear the datapath right now, since that will destroy stats. Delay
  // clearing the datapath until it gets reinitialized on the next
  // invocation.
}

MemoryOpType HybridDatapath::getMemoryOpType(const std::string& array_label) {
  auto it = user_params.partition.find(array_label);
  if (it != user_params.partition.end()) {
    PartitionEntry entry = it->second;
    switch (entry.memory_type) {
      case spad:
        return Scratchpad;
      case reg:
        return Register;
      case dma:
        return Dma;
      case acp:
        return ACP;
      case cache:
        return Cache;
      default:
        throw IllegalHostMemoryAccessException(array_label);
    }
  } else {
    throw UnknownArrayException(array_label);
  }
  return NumMemoryOpTypes;
}

MemoryOpType HybridDatapath::getMemoryOpType(ExecNode* node) {
  if (node->is_dma_load() || node->is_dma_store() || node->is_dma_fence())
    return Dma;
  if (node->is_set_ready_bits())
    return ReadyBits;

  if (node->is_host_mem_op()) {
    HostMemAccess* mem_access = node->get_host_mem_access();
    bool isLoad = node->is_host_load();
    const std::string& array_label = isLoad ? mem_access->src_var->get_name()
                                            : mem_access->dst_var->get_name();
    return getMemoryOpType(array_label);
  } else if (node->is_memory_op()) {
    const std::string& array_label = node->get_array_label();
    auto it = user_params.partition.find(array_label);
    return getMemoryOpType(array_label);
  } else {
    fatal("Unexpected memory op! It should either be a host memory op or a "
          "local memory op.\n");
  }
  return NumMemoryOpTypes;
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
  const std::string& array_name = node->get_array_label();
  unsigned array_name_index = node->get_partition_index();
  MemAccess* mem_access = node->get_mem_access();
  Addr vaddr = mem_access->vaddr;
  bool isLoad = node->is_load_op();
  try {
    bool satisfied = scratchpad->canServicePartition(
                         array_name, array_name_index, vaddr, isLoad);
    if (!satisfied)
      return false;
  } catch (UnknownArrayException& e) {
    fatal("At node %d: tried to access array \"%s\", which does not exist!\n",
          node->get_node_id(), array_name);
  } catch (ArrayAccessException& e) {
    fatal("At node %d: %s\n", node->get_node_id(), e.what());
  }

  markNodeStarted(node);
  if (isLoad) {
    scratchpad->increment_loads(array_name, array_name_index);
  } else {
    size_t size = mem_access->size;
    uint8_t* data = mem_access->data();
    try {
      scratchpad->writeData(array_name, vaddr, data, size);
    } catch (ArrayAccessException& e) {
      fatal("At node %d, attempting to access array %d, trace addr %#x, size "
            "%d: %s", node->get_node_id(), array_name, vaddr, size, e.what());
    }
    scratchpad->increment_stores(array_name, array_name_index);
  }
  return true;
}

bool HybridDatapath::handleReadyBitAccess(ExecNode* node) {
  ReadyBitAccess* access = node->get_ready_bit_access();
  const std::string& array_name = access->array->get_name();
  Addr start_addr = access->vaddr;
  size_t size = access->size;
  bool reset_ready_bits = (access->value == 0);
  try {
    if (reset_ready_bits)
      scratchpad->resetReadyBitRange(array_name, start_addr, size);
    else
      scratchpad->setReadyBitRange(array_name, start_addr, size);
  } catch (UnknownArrayException& e) {
    fatal("At node %d: tried to set ready bits on array \"%s\", which does not "
          "exist!\n", node->get_node_id(), array_name);
  }
  return true;
}

bool HybridDatapath::handleDmaMemoryOp(ExecNode* node) {
  if (node->is_dma_fence())
    return true;

  MemAccessStatus status = Invalid;
  unsigned node_id = node->get_node_id();
  if (inflight_dma_nodes.find(node_id) == inflight_dma_nodes.end())
    inflight_dma_nodes[node_id] = status;
  else
    status = inflight_dma_nodes.at(node_id);
  HostMemAccess* mem_access = node->get_host_mem_access();
  if (status == Invalid && inflight_dma_nodes.size() < maxInflightNodes) {
    /* A DMA load needs to be preceded by a cache flush of the buffer being
     * sent, while a DMA store needs to be preceded by a cache invalidate of the
     * receiving buffer.
     */
    size_t size = mem_access->size;
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
      ExecNode* node = program.nodes.at(node_id);
      node->set_dma_scheduling_delay_cycle(num_cycles);
    }
    inflight_dma_nodes[node_id] = WaitingForDmaSetup;
    dmaWaitingQueue.push_back(std::make_pair(node_id, delay));
    return false;  // DMA op not completed. Move on to the next node.
  } else if (status == Returned) {
    inflight_dma_nodes.erase(node_id);
    return true;  // DMA op completed.
  } else {
    // Still waiting from the cache or for DMA setup to finish. Move on to the next node.
    return false;
  }
}

bool HybridDatapath::handleCacheMemoryOp(ExecNode* node) {
  unsigned node_id = node->get_node_id();
  if (!cache_queue.canIssue()) {
    DPRINTF(HybridDatapathVerbose,
            "Unable to service cache request for node %d: "
            "out of cache queue bandwidth.\n",
            node_id);
    return false;
  }

  MemAccess* mem_access = program.nodes[node_id]->get_mem_access();
  bool isLoad = node->is_load_op();
  Addr vaddr = mem_access->vaddr;
  MemoryQueueEntry* entry = cache_queue.findMatch(vaddr, isLoad);
  if (!entry)
    entry = cache_queue.allocateEntry(vaddr, mem_access->size, isLoad);
  if (!entry) {
    DPRINTF(HybridDatapathVerbose,
            "Unable to service new cache request for node %d: "
            "cache queue is full.\n",
            node_id);
    return false;
  } else {
    entry->type = Cache;
  }

  // At this point, we have a valid entry.

  if (entry->status == Invalid) {
    int size = mem_access->size;
    SrcTypes::Variable* var =
        srcManager.get<SrcTypes::Variable>(node->get_array_label());
    if (dtb.canRequestTranslation()) {
      bool issued_tlb_req = false;
      try {
        issued_tlb_req = issueTLBRequestTiming(vaddr, size, isLoad, var);
      } catch (AladdinException& e) {
        fatal("An error occurred during cache access to trace virtual address "
              "%#x at node %d: %s\n",
              vaddr, node_id, e.what());
      }
      if (!issued_tlb_req)
        return false;
      markNodeStarted(node);
      dtb.incrementRequestCounter();
      entry->status = Translating;
      DPRINTF(HybridDatapath,
              "node: %d, trace vaddr = %x, is translating\n",
              node_id,
              vaddr);
    } else {
      DPRINTF(HybridDatapathVerbose, "TLB cannot service node %d\n",
              node->get_node_id());
    }
    return false;
  } else if (entry->status == Translated || entry->status == Retry) {
    Addr paddr = entry->paddr;
    int size = mem_access->size;
    uint8_t* data = mem_access->data();

    IssueResult result =
        issueCacheRequest(vaddr, paddr, size, isLoad, node_id, data);
    if (result == Accepted) {
      entry->status = WaitingFromCache;
      entry->when_issued = curTick();
      dcache_queue_time.sample(entry->when_issued - entry->when_allocated);
      cache_queue.incrementIssuedThisCycle();
      DPRINTF(HybridDatapath,
              "node:%d, vaddr = 0x%x, paddr = 0x%x is accessing cache\n",
              node_id,
              vaddr,
              paddr);
    } else if (result == WillRetry) {
      entry->status = Retry;
    }
    return false;
  } else if (entry->status == Returned) {
    // Memory ops that have returned will be freed at the end of the cycle.
    return true;
  } else if (entry->status == Translating) {
    DPRINTF(HybridDatapathVerbose,
            "Node %d is still waiting for TLB response.\n",
            node->get_node_id());
    return false;
  } else if (entry->status == WaitingFromCache) {
    DPRINTF(HybridDatapathVerbose,
            "Node %d is still waiting for cache response.\n",
            node->get_node_id());
    return false;
  } else {
    panic("Invalid memory status for cache operation.");
  }
}

MemoryQueueEntry* HybridDatapath::createAndQueueAcpEntry(
    Addr vaddr,
    int size,
    bool isLoad,
    const std::string& array_label,
    unsigned node_id) {
  // For loads: we don't need to ask for ownership at all. Instead, we collapse
  // simultaneous requests to the same cache line (aka an MSHR).
  //
  // For stores: we issue separate ownership requests for each request.
  MemoryQueueEntry* entry = cache_queue.findMatch(vaddr, isLoad);
  if (!entry) {
    entry = cache_queue.allocateEntry(vaddr, size, isLoad);
    entry->type = ACP;
    if (use_acp_cache) {
      // If we have the middle cache, we don't need to handle ownership requests
      // ourselves. Just go ahead and issue the read or write.
      // TODO: We should remove the ACP cache and require the user to use Ruby
      // for ACP simulations.
      entry->status = ReadyToIssue;
    } else {
      // We need to explicitly request ownership of a cache line on stores.
      entry->status = isLoad ? ReadyToIssue : ReadyToRequestOwnership;
    }
    // ACP works with physical addresses, which the trace cannot possibly have,
    // so to model this faithfully, we perform the address translation in zero
    // time. Make sure we use the original virtual address.
    SrcTypes::Variable* var = srcManager.get<SrcTypes::Variable>(array_label);
    try {
      AladdinTLBResponse translation =
          getAddressTranslation(vaddr, size, isLoad, var);
      entry->paddr = translation.paddr;
    } catch (AladdinException& e) {
      fatal("An error occurred during ACP access at node %d: %s\n",
            node_id,
            e.what());
    }
  }
  return entry;
}

bool HybridDatapath::updateAcpEntryStatus(MemoryQueueEntry* entry,
                                          ExecNode* node) {
  Addr vaddr = entry->vaddr;
  Addr paddr = entry->paddr;
  int size = entry->size;
  bool isLoad = entry->isLoad;
  unsigned node_id = node->get_node_id();
  if (entry->status == ReadyToRequestOwnership) {
    IssueResult result = issueOwnershipRequest(vaddr, paddr, size, node_id);
    if (result == Accepted) {
      entry->status = WaitingForOwnership;
      cache_queue.incrementIssuedThisCycle();
      DPRINTF(HybridDatapath, "node:%d, vaddr = 0x%x, paddr = 0x%x is waiting "
                              "to acquire ownership\n",
              node_id, vaddr, paddr);
    } else if (result == WillRetry) {
      // The port will retry the packet.
      entry->status = Retry;
    } else {
      // The datapath will do the retry, so nothing needs to be done.
    }
    return false;
  } else if (entry->status == ReadyToIssue) {
    if (acpPort.inRetry())
      return false;
    uint8_t* data;
    if (node->is_memory_op()) {
      data = node->get_mem_access()->data();
    } else {
      // This data will be deleted upon cache response.
      data = new uint8_t[size];
      if (!isLoad) {
        HostMemAccess* mem_access = node->get_host_mem_access();
        Addr offset = vaddr - mem_access->dst_addr;
        Addr accel_addr = offset + mem_access->src_addr;
        const std::string& array_label = mem_access->src_var->get_name();
        try {
          scratchpad->readData(array_label, accel_addr, size, data);
        } catch (ArrayAccessException& e) {
          fatal("When reading from array %s at node %d: %s\n",
                node->get_array_label(), node->get_node_id(), e.what());
        }
      }
    }
    IssueResult result =
        issueAcpRequest(vaddr, paddr, size, isLoad, node_id, data);
    if (result == Accepted) {
      entry->status = WaitingFromCache;
      entry->when_issued = curTick();
      acp_queue_time.sample(entry->when_issued - entry->when_allocated);
      cache_queue.incrementIssuedThisCycle();
      DPRINTF(
          HybridDatapath,
          "node:%d, vaddr = 0x%x, paddr = 0x%x is accessing cache via ACP\n",
          node_id, vaddr, paddr);
    } else if (result == WillRetry) {
      entry->status = Retry;
    }
    return false;
  } else if (entry->status == WaitingFromCache) {
    DPRINTF(HybridDatapathVerbose,
            "Node %d is still waiting for L2 cache response.\n",
            node_id);
    return false;
  } else if (entry->status == WaitingForOwnership) {
    DPRINTF(HybridDatapathVerbose,
            "Node %d is still waiting to acquire ownership.\n",
            node_id);
    return false;
  } else if (entry->status == Retry) {
    DPRINTF(HybridDatapathVerbose,
            "Node %d is waiting to be retried.\n",
            node_id);
    return false;
  } else if (entry->status == Returned) {
    // Memory ops that have returned will be freed at the end of the cycle.
    return true;
  } else {
    panic("Invalid memory status for ACP operation.");
  }
}

bool HybridDatapath::handleAcpMemoryOp(ExecNode* node) {
  if (!acp_enabled) {
    fatal("Attempted to perform an ACP access when ACP was not enabled on this "
          "system! Add \"enable_acp = True\" to the gem5.cfg file and try "
          "again.");
  }

  unsigned node_id = node->get_node_id();
  if (!cache_queue.canIssue()) {
    DPRINTF(HybridDatapathVerbose, "Unable to service ACP request for node %d: "
                                   "out of cache queue bandwidth.\n",
            node_id);
    return false;
  }

  if (!node->started())
    markNodeStarted(node);

  // There are two ways to access host memory via ACP: chunk-level access and
  // fine-grained cacheline-level access. Chunk-level accesses will use
  // the hostLoad/hostStore instructions, and cacheline-level accesses will use
  // the normal Load/Store.
  if (node->is_host_mem_op()) {
    // This is a chunk-level ACP access.
    if (inflight_burst_nodes.find(node_id) == inflight_burst_nodes.end()) {
      // This is a new op. Break up the chunk into cacheline size requests and
      // insert them into the cache queue.
      bool isLoad = node->is_host_load();
      HostMemAccess* mem_access = node->get_host_mem_access();
      size_t size = mem_access->size;  // In bytes.
      Addr src_vaddr = mem_access->src_addr;
      Addr dst_vaddr = mem_access->dst_addr;
      Addr host_vaddr = isLoad ? src_vaddr : dst_vaddr;
      const std::string& array_label = isLoad ? mem_access->src_var->get_name()
                                              : mem_access->dst_var->get_name();
      for (ChunkGenerator gen(host_vaddr, size, cacheLineSize); !gen.done();
           gen.next()) {
        inflight_burst_nodes[node_id].push_back(createAndQueueAcpEntry(
            gen.addr(), gen.size(), isLoad, array_label, node_id));
      }
    }
    if (inflight_burst_nodes[node_id].size() > 0) {
      // This node has outstanding requests.
      for (auto entry : inflight_burst_nodes[node_id]) {
        updateAcpEntryStatus(entry, node);
        if (!cache_queue.canIssue()) {
          DPRINTF(HybridDatapathVerbose,
                  "Out of Cache queue bandwidth while issuing ACP requests of "
                  "host node %d.\n",
                  node_id);
          return false;
        }
      }
      return false;
    } else {
      DPRINTF(
          HybridDatapath, "Host memory op completes for node %d.\n", node_id);
      inflight_burst_nodes.erase(node_id);
      // All the requests of this node have completed.
      return true;
    }
  } else if (node->is_memory_op()) {
    // This is a cacheline-level ACP access.
    MemAccess* mem_access = node->get_mem_access();
    bool isLoad = node->is_load_op();

    // TODO: Use the simulation virtual address. This is the TRACE address, so
    // if we are going to compute aligned addresses, then the trace needs to be
    // generated with aligned addresses too. That's probably not an onerous
    // requirement, but we should not depend on the trace address for anything
    // performance related.
    Addr vaddr = mem_access->vaddr;
    int size = mem_access->size;
    MemoryQueueEntry* entry = createAndQueueAcpEntry(
        vaddr, size, isLoad, node->get_array_label(), node_id);
    return updateAcpEntryStatus(entry, node);
  } else {
    fatal("Unexpected ACP access op, node id %d!\n", node_id);
  }
}

Addr HybridDatapath::translateAtomic(Addr vaddr, int size) {
  Addr paddr;
  try {
    paddr = dtb.translateInvisibly(vaddr, size);
  } catch (AladdinException& e) {
    fatal("An error occurred during DMA address translation: %s\n", e.what());
  }
  return paddr;
}

// Issue a DMA request for memory.
void HybridDatapath::issueDmaRequest(unsigned node_id) {
  ExecNode *node = program.nodes.at(node_id);
  markNodeStarted(node);
  bool isLoad = node->is_dma_load();
  HostMemAccess* mem_access = node->get_host_mem_access();
  size_t size = mem_access->size;  // In bytes.
  Addr src_vaddr = mem_access->src_addr;
  Addr dst_vaddr = mem_access->dst_addr;
  DPRINTF(HybridDatapath,
          "issueDmaRequest: src_addr: %#x, dst_addr: %#x, size: %u\n",
          src_vaddr, dst_vaddr, size);
  /* Assigning the array label can (and probably should) be done in the
   * optimization pass instead of the scheduling pass. */
  Addr accel_addr = isLoad ? dst_vaddr : src_vaddr;

  MemoryType mtype;
  try {
    auto part_it = user_params.getArrayConfig(accel_addr);
    mtype = part_it->second.memory_type;
    node->set_array_label(part_it->first);
  } catch (UnknownArrayException& e) {
    fatal("During DMA at node %d: %s", node_id, e.what());
  }
  fatal_if(!(mtype == spad || mtype == reg),
           "At node %d: DMA request to %s must be to/from a scratchpad or "
           "register, but we got %s\n",
           node_id, node->get_array_label(), memoryTypeToString(mtype));
  if (mtype == spad)
    incrementDmaScratchpadAccesses(node->get_array_label(), size, isLoad);
  else
    registers.getRegister(node->get_array_label())
        ->increment_dma_accesses(isLoad);

  // Update the tracking structures.
  inflight_dma_nodes.at(node_id) = WaitingFromDma;

  uint8_t* data = new uint8_t[size];
  if (!isLoad) {
    try {
      scratchpad->readData(node->get_array_label(), accel_addr, size, data);
    } catch (ArrayAccessException& e) {
      fatal("When reading from array %s at node %d: %s\n",
            node->get_array_label(), node->get_node_id(), e.what());
    }
  }

  // Because splitAndSendDmaRequest only receives the simulated start virtual
  // address, here we do one translation to get that virtual address.
  Addr traceStartVaddr = isLoad ? src_vaddr : dst_vaddr;
  SrcTypes::Variable* var = isLoad ? mem_access->src_var : mem_access->dst_var;
  AladdinTLBResponse translation =
      getAddressTranslation(traceStartVaddr, size, isLoad, var);
  Addr simStartVaddr = translation.vaddr;
  AladdinDmaEvent* dmaEvent = new AladdinDmaEvent(this, simStartVaddr, node_id);
  splitAndSendDmaRequest(simStartVaddr, size, isLoad, data, dmaEvent);
}

/* Mark the DMA request node as having completed. */
void HybridDatapath::dmaCompleteCallback(DmaEvent* event) {
  AladdinDmaEvent* aladdinEvent = dynamic_cast<AladdinDmaEvent*>(event);
  unsigned node_id = aladdinEvent->get_node_id();
  assert(inflight_dma_nodes.find(node_id) != inflight_dma_nodes.end());
  DPRINTF(HybridDatapath, "%s for node %d.\n", __func__, node_id);
  inflight_dma_nodes.at(node_id) = Returned;
}

void HybridDatapath::addDmaNodeToIssueQueue(unsigned node_id) {
  assert(program.nodes.at(node_id)->is_dma_op() &&
         "Cannot add non-DMA node to DMA issue queue!");
  DPRINTF(HybridDatapath, "Adding DMA node %d to DMA issue queue.\n", node_id);
  assert(dmaIssueQueue.find(node_id) == dmaIssueQueue.end());
  dmaIssueQueue.insert(node_id);
}

void HybridDatapath::sendFinishedSignal() {
  Request::Flags flags = 0;
  size_t size = 4;  // 32 bit integer.
  uint8_t* data = new uint8_t[size];
  // Set some sentinel value.
  for (int i = 0; i < size; i++)
    data[i] = 0x13;
  auto req =
      std::make_shared<Request>(finish_flag, size, flags, getCacheMasterId());
  req->setContext(context_id);  // Only needed for prefetching.
  MemCmd::Command cmd = MemCmd::WriteReq;
  PacketPtr pkt = new Packet(req, cmd);
  pkt->dataStatic<uint8_t>(data);
  DatapathSenderState* state = new DatapathSenderState(true, Cache);
  pkt->pushSenderState(state);

  if (!cachePort.sendTimingReq(pkt)) {
    assert(!cachePort.inRetry());
    cachePort.setRetryPkt(pkt);
    DPRINTF(HybridDatapath, "Sending finished signal failed, retrying.\n");
  } else {
    DPRINTF(HybridDatapath, "Sent finished signal.\n");
  }
}

void HybridDatapath::cacheRetryCallback(PacketPtr retryPkt) {
  updateCacheRequestStatusOnRetry(retryPkt);
}

void HybridDatapath::cacheRespCallback(PacketPtr pkt) {
  DatapathSenderState* state = pkt->findNextSenderState<DatapathSenderState>();
  assert(state && "Packet did not contain a DatapathSenderState!");
  if (!state->is_ctrl_signal) {
    updateCacheRequestStatusOnResp(pkt, state);
  } else {
    ackedFinishSignal = true;
    schedule(tickEvent, clockEdge(Cycles(1)));
    DPRINTF(HybridDatapath,
            "cacheRespCallback for control signal access: %#x\n",
            pkt->getAddr());
  }
}

bool HybridDatapath::issueTLBRequestTiming(
    Addr trace_addr, unsigned size, bool isLoad, SrcTypes::Variable* var) {
  DPRINTF(HybridDatapath, "%s for trace addr: %#x\n", __func__, trace_addr);
  PacketPtr data_pkt = createTLBRequestPacket(trace_addr, size, isLoad, var);

  if (dtb.translateTiming(data_pkt)) {
    return true;
  } else {
    // Delete the entire packet; we'll recreate it when we retry the TLB.
    delete data_pkt->popSenderState();
    delete data_pkt;
    return false;
  }
}

AladdinTLBResponse HybridDatapath::getAddressTranslation(
    Addr addr, unsigned size, bool isLoad, SrcTypes::Variable* var) {
  PacketPtr data_pkt = createTLBRequestPacket(addr, size, isLoad, var);
  dtb.translateInvisibly(data_pkt);
  // data_pkt->data now contains the translated address. Make a local copy and
  // return it.
  AladdinTLBResponse translation = *data_pkt->getPtr<AladdinTLBResponse>();

  delete data_pkt->popSenderState();
  delete data_pkt;
  return translation;
}

PacketPtr HybridDatapath::createTLBRequestPacket(
    Addr trace_addr, unsigned size, bool isLoad, SrcTypes::Variable* var) {
  Request::Flags flags = 0;
  // Constructor for physical request only
  auto req =
      std::make_shared<Request>(trace_addr, size, flags, getCacheMasterId());
  MemCmd command = isLoad ? MemCmd::ReadReq : MemCmd::WriteReq;
  PacketPtr data_pkt = new Packet(req, command);

  AladdinTLBResponse* translation = new AladdinTLBResponse();
  AladdinTLB::TLBSenderState* state = new AladdinTLB::TLBSenderState(var);
  data_pkt->dataStatic<AladdinTLBResponse>(translation);
  data_pkt->pushSenderState(state);

  return data_pkt;
}

void HybridDatapath::completeTLBRequest(PacketPtr pkt, bool was_miss) {
  // TLB packets will never be sent out to the memory system, and the only
  // sender states we attach are our own.
  AladdinTLB::TLBSenderState* state =
      safe_cast<AladdinTLB::TLBSenderState*>(pkt->popSenderState());
  DPRINTF(HybridDatapath,
          "completeTLBRequest for paddr: %#x %s\n",
          pkt->getAddr(),
          pkt->cmdString());
  Addr vaddr = pkt->getAddr();  // The trace address.
  bool isLoad = pkt->isRead();
  MemoryQueueEntry* entry = cache_queue.findMatch(vaddr, isLoad);
  panic_if(!entry || entry->status != Translating,
           "Unable to find matching cache queue entry!");

  /* If the TLB missed, we need to retry the complete instruction, as the dcache
   * state may have changed during the waiting time, so we go to Invalid
   * instead of Translated. The next translation request should hit.
   */
  if (was_miss) {
    entry->status = Invalid;
  } else {
    // Translations are actually the complete memory address, not just the page
    // number, because of the trace to virtual address translation.
    Addr paddr = pkt->getPtr<AladdinTLBResponse>()->paddr;
    entry->status = Translated;
    entry->paddr = paddr;
    DPRINTF(
        HybridDatapath, "Translated: vaddr = %#x, paddr = %#x\n", vaddr, paddr);
  }
  delete state;
  delete pkt;
}

HybridDatapath::IssueResult HybridDatapath::issueCacheRequest(Addr vaddr,
                                                              Addr paddr,
                                                              unsigned size,
                                                              bool isLoad,
                                                              unsigned node_id,
                                                              uint8_t* data) {
  return issueCacheOrAcpRequest(
      Cache, vaddr, paddr, size, isLoad, node_id, data);
}

HybridDatapath::IssueResult HybridDatapath::issueAcpRequest(Addr vaddr,
                                                            Addr paddr,
                                                            unsigned size,
                                                            bool isLoad,
                                                            unsigned node_id,
                                                            uint8_t* data) {
  return issueCacheOrAcpRequest(
      ACP, vaddr, paddr, size, isLoad, node_id, data);
}

HybridDatapath::IssueResult HybridDatapath::issueCacheOrAcpRequest(
    MemoryOpType op_type,
    Addr vaddr,
    Addr paddr,
    unsigned size,
    bool isLoad,
    unsigned node_id,
    uint8_t* data) {
  using namespace SrcTypes;
  CachePort& port = op_type == Cache ? cachePort : acpPort;

  if (port.inRetry()) {
    DPRINTF(HybridDatapathVerbose, "%s: %s port in retry.\n",
            op_type == Cache ? "Cache" : "ACP", __func__);
    return DidNotAttempt;
  }

  MasterID id = op_type == Cache ? getCacheMasterId() : getAcpMasterId();

  Request::Flags flags = 0;
  /* To use strided prefetching, we need to include a "PC" so the prefetcher
   * can predict memory behavior similar to how branch predictors work. We
   * don't have real PCs in aladdin so we'll just hash the unique id of the
   * node.  */
  DynamicInstruction inst = program.nodes.at(node_id)->get_dynamic_instruction();
  Addr pc = static_cast<Addr>(std::hash<DynamicInstruction>()(inst));
  auto req = std::make_shared<Request>(paddr, size, flags, id, curTick(), pc);
  /* The context id and thread ids are needed to pass a few assert checks in
   * gem5, but they aren't actually required for the mechanics of the memory
   * checking itself. This has to be set outside of the constructor or the
   * address will be interpreted as a virtual, not physical. */
  req->setContext(context_id);

  MemCmd command = isLoad ? MemCmd::ReadReq : MemCmd::WriteReq;
  PacketPtr data_pkt = new Packet(req, command);
  data_pkt->dataStatic(data);

  DatapathSenderState* state = new DatapathSenderState(node_id, vaddr, op_type);
  data_pkt->pushSenderState(state);

  if (!port.sendTimingReq(data_pkt)) {
    assert(!port.inRetry());
    assert(data_pkt->isRequest());
    port.setRetryPkt(data_pkt);
    DPRINTF(HybridDatapathVerbose, "%s port is blocked. Will retry node %d.\n",
            op_type == Cache ? "Cache" : "ACP", node_id);
    return WillRetry;
  }
  return Accepted;
}

HybridDatapath::IssueResult HybridDatapath::issueOwnershipRequest(
    Addr vaddr, Addr paddr, unsigned size, unsigned node_id) {
  using namespace SrcTypes;

  if (acpPort.inRetry()) {
    DPRINTF(HybridDatapathVerbose, "ACP port is blocked. Will retry node %d.\n",
            node_id);
    return DidNotAttempt;
  }

  // In order to obtain ownership of a cache line, we need an aligned access.
  paddr = toCacheLineAddr(paddr);
  size = cacheLineSize;

  RequestPtr req = NULL;
  Request::Flags flags = 0;
  DynamicInstruction inst = program.nodes.at(node_id)->get_dynamic_instruction();
  Addr pc = static_cast<Addr>(std::hash<DynamicInstruction>()(inst));
  req = std::make_shared<Request>(
      paddr, size, flags, getAcpMasterId(), curTick(), pc);
  req->setContext(context_id);

  // The command that comes closest to emulating ACP behavior is ReadExReq,
  // which gives us ownership of this cache line, returns the latest version of
  // the data, and invalidates all other copies. It does not force a writeback
  // to L2, however; that can only be triggered by the cache that is doing the
  // writeback in the first place.
  MemCmd command = MemCmd::ReadExReq;

  PacketPtr data_pkt = new Packet(req, command);
  DatapathSenderState* state = new DatapathSenderState(node_id, vaddr, ACP);
  data_pkt->pushSenderState(state);
  uint8_t* data = new uint8_t[size];
  data_pkt->dataDynamic<uint8_t>(data);

  if (!acpPort.sendTimingReq(data_pkt)) {
    // blocked, retry
    assert(!acpPort.inRetry());
    acpPort.setRetryPkt(data_pkt);
    DPRINTF(HybridDatapathVerbose, "ACP is blocked. Will retry later...\n");
    return WillRetry;
  }
  DPRINTF(HybridDatapath,
          "Node id %d invalidation of paddr %#x issued to cache.\n", node_id,
          paddr);

  return Accepted;
}

void HybridDatapath::updateCacheRequestStatusOnResp(
    PacketPtr pkt, DatapathSenderState* state) {
  unsigned node_id = state->node_id;
  Addr vaddr = state->vaddr;
  DPRINTF(HybridDatapath, "%s for node_id %d, %s\n", __func__, node_id,
          pkt->print());

  bool isLoad = (pkt->cmd == MemCmd::ReadResp);
  MemoryQueueEntry* entry = cache_queue.findMatch(vaddr, isLoad);
  panic_if(!entry, "Did not find a cache queue entry for vaddr %#x\n", vaddr);
  if (entry->status == WaitingFromCache) {
    entry->status = Returned;
    Tick elapsed = curTick() - entry->when_issued;
    ExecNode* node = getProgram().nodes.at(node_id);
    if (node->is_host_mem_op()) {
      // If this is a bursty memory op, remove the request from its request
      // lists and write the data into the scratchpad if it's a hostLoad.
      inflight_burst_nodes[node_id].remove(entry);
      uint8_t* data = pkt->getPtr<uint8_t>();
      if (isLoad) {
        assert(data != nullptr && "Packet data is null!");
        HostMemAccess* mem_access = node->get_host_mem_access();
        const std::string& array_label = mem_access->dst_var->get_name();
        Addr offset = vaddr - mem_access->src_addr;
        Addr spadAddr = mem_access->dst_addr + offset;
        unsigned size = pkt->req->getSize();  // in bytes
        try {
          scratchpad->writeData(array_label, spadAddr, data, size);
        } catch (ArrayAccessException& e) {
          fatal("When writing to array %s at node %d: %s\n", array_label,
                node_id, e.what());
        }
      }
      delete[] data;
    }
    DPRINTF(HybridDatapath,
            "setting %s cache queue entry for vaddr %#x to Returned.\n",
            isLoad ? "load" : "store",
            vaddr);
    Stats::Scalar* mem_stat_count;
    Stats::Histogram* mem_stat_latency;
    switch (entry->type) {
      case Cache:
        mem_stat_count = isLoad ? &dcache_loads : &dcache_stores;
        mem_stat_latency = &dcache_latency;
        break;
      case ACP:
        mem_stat_count = isLoad ? &acp_loads : &acp_stores;
        mem_stat_latency = &acp_latency;
        break;
      default:
        panic("Unexpected memory operation type trying to update the cacne "
              "queue!");
    }
    (*mem_stat_count)++;
    mem_stat_latency->sample(elapsed);
  } else if (entry->status == WaitingForOwnership) {
    entry->status = ReadyToIssue;
    DPRINTF(HybridDatapath,
            "setting cache queue entry for address %#x to ReadyToIssue.\n",
            vaddr);
  }
}

void HybridDatapath::updateCacheRequestStatusOnRetry(PacketPtr pkt) {
  DatapathSenderState* state = pkt->findNextSenderState<DatapathSenderState>();
  assert(state && "Retry packet is missing DatapathSenderState!");
  Addr vaddr = state->vaddr;
  MemoryOpType mem_type = state->mem_type;
  DPRINTF(
      HybridDatapath,
      "node:%d, vaddr = 0x%x, paddr = 0x%x, %s request retried successfully.\n",
      state->node_id, vaddr, pkt->getAddr(),
      mem_type == Cache ? "Cache" : "ACP");

  // If we receive a ReadExReq or ReadExReq, that means that the trace vaddr
  // associated with the request was for a STORE, and stores do not (currently)
  // get merged into a single cache queue entry.
  bool isLoad = pkt->cmd == MemCmd::ReadReq || pkt->cmd == MemCmd::ReadResp;
  MemoryQueueEntry* entry = cache_queue.findMatch(vaddr, isLoad);
  panic_if(entry->status != Retry,
           "%s called with packet %s, which is not in retry!\n", __func__,
           pkt->print());

  // This packet could be either a request or a response! After calling
  // sendTimingReq(), the original packet could have already been transformed
  // into a response, but the response is not meant to be handled until
  // recvTimingResp() is called (potentially after some delay).
  switch (pkt->cmd.toInt()) {
    case MemCmd::ReadExReq:
    case MemCmd::ReadExResp:
      entry->status = WaitingForOwnership;
      break;
    case MemCmd::ReadReq:
    case MemCmd::ReadResp:
    case MemCmd::WriteReq:
    case MemCmd::WriteLineReq:
    case MemCmd::WriteResp:
      entry->status = WaitingFromCache;
      entry->when_issued = curTick();
      break;
    default:
      panic("%s called with an unexpected packet %s\n",
            __func__, pkt->print());
  }
}

/* Receiving response from DMA. */
void HybridDatapath::dmaRespCallback(PacketPtr pkt) {
  // Get the DMA sender state to retrieve node id, so we can get the virtual
  // address (the packet address is possibly physical).
  AladdinDmaEvent* event =
      dynamic_cast<AladdinDmaEvent*>(DmaPort::getPacketCompletionEvent(pkt));
  assert(event != nullptr && "Packet completion event is not a DmaEvent object!");

  unsigned node_id = event->get_node_id();
  ExecNode * node = getProgram().nodes.at(node_id);
  assert(node != nullptr && "DMA node id is invalid!");

  HostMemAccess* mem_access = node->get_host_mem_access();
  Addr src_vaddr_base = mem_access->src_addr;
  Addr dst_vaddr_base = mem_access->dst_addr;

  // This will compute the offset of THIS packet from the base address. We first
  // calculate the offset within the page, then we retrieve the request offset
  // (e.g., the N'th page of the overall DMA request) from the DMA event.
  Addr paddr = pkt->getAddr();
  Addr paddr_base = DmaPort::getPacketAddr(pkt);
  Addr pageOffset = paddr - paddr_base;
  Addr pkt_offset = pageOffset + event->getReqOffset();
  Addr vaddr =
      (node->is_dma_load() ? dst_vaddr_base : src_vaddr_base) + pkt_offset;

  std::string array_label = node->get_array_label();
  unsigned size = pkt->req->getSize(); // in bytes
  DPRINTF(HybridDatapath, "Receiving DMA response for node %d, address %#x "
                          "with label %s and size %d.\n",
          node_id, vaddr, array_label.c_str(), size);
  if (node->is_dma_load() && isReadyMode())
    scratchpad->setReadyBitRange(array_label, vaddr, size);

  // For dmaLoads, write the data into the scratchpad.
  if (node->is_dma_load()) {
    uint8_t* data = pkt->getPtr<uint8_t>();
    assert(data != nullptr && "Packet data is null!");
    try {
      scratchpad->writeData(array_label, vaddr, data, size);
    } catch (ArrayAccessException& e) {
      fatal("When writing to array %s at node %d: %s\n", array_label, node_id,
            e.what());
    }
  }
}

void HybridDatapath::resetCacheCounters() {
  dtb.resetRequestCounter();
  cache_queue.resetIssuedThisCycle();
}

void HybridDatapath::regStats() {
  using namespace Stats;
  DPRINTF(HybridDatapath, "Registering stats.\n");
  dcache_loads.name("system." + datapath_name + ".total_dcache_loads")
      .desc("Total number of dcache loads")
      .flags(total | nonan);
  dcache_stores.name("system." + datapath_name + ".total_dcache_stores")
      .desc("Total number of dcache stores.")
      .flags(total | nonan);
  dcache_latency.name("system." + datapath_name + ".dcache_latency")
      .init(30)
      .desc("Histogram of dcache total access latency")
      .flags(pdf);
  dcache_queue_time.name("system." + datapath_name + ".dcache_queue_time")
      .init(30)
      .desc("Histogram of time dcache request spent in queue")
      .flags(pdf);
  acp_loads.name("system." + datapath_name + ".total_acp_loads")
      .desc("Total number of ACP loads")
      .flags(total | nonan);
  acp_stores.name("system." + datapath_name + ".total_acp_stores")
      .desc("Total number of ACP stores.")
      .flags(total | nonan);
  acp_latency.name("system." + datapath_name + ".acp_latency")
      .init(30)
      .desc("Histogram of ACP transaction total latency")
      .flags(pdf);
  acp_queue_time.name("system." + datapath_name + ".acp_queue_time")
      .init(30)
      .desc("Histogram of time ACP request spent in queue")
      .flags(pdf);
  dma_setup_cycles.name("system." + datapath_name + ".dma_setup_cycles")
      .desc("Total number of cycles spent on setting up DMA transfers.")
      .flags(total | nonan);
  acc_sim_cycles.name("system." + datapath_name + ".sim_cycles")
      .desc("Total accelerator cycles")
      .flags(total | nonan);
  Gem5Datapath::regStats();
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
           "max_dma_requests, dma_setup_overhead, cache_size, cache_line_sz, "
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
        << spadPort.max_req << "," << dmaSetupOverhead << ",\"" << cacheSize
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
  float cq_avg_power = 0, cq_avg_dynamic = 0, cq_avg_leak = 0;

  computeCactiResults();
  getAverageCachePower(
      cycles, &cache_avg_power, &cache_avg_dynamic, &cache_avg_leak);
  getAverageSpadPower(
      cycles, &spad_avg_power, &spad_avg_dynamic, &spad_avg_leak);
  getAverageCacheQueuePower(
      cycles, &cq_avg_power, &cq_avg_dynamic, &cq_avg_leak);
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

  avg_cache_ac_pwr = (dcache_loads.value() * cache_readEnergy +
                      dcache_stores.value() * cache_writeEnergy) /
                     (cycles * user_params.cycle_time);
  avg_cache_leak = cache_leakagePower;
  avg_cache_pwr = avg_cache_ac_pwr + avg_cache_leak;

  dtb.getAveragePower(cycles, user_params.cycle_time, &avg_tlb_pwr,
                      &avg_tlb_ac_pwr, &avg_tlb_leak);

  *avg_dynamic = avg_cache_ac_pwr;
  *avg_leak = avg_cache_leak;
  *avg_power = avg_cache_pwr;

  // TODO: Write this detailed data to the database as well.
  std::string power_file_name = benchName + "_cache_stats.txt";
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

void HybridDatapath::getAverageCacheQueuePower(unsigned int cycles,
                                               float* avg_power,
                                               float* avg_dynamic,
                                               float* avg_leak) {
  cache_queue.getAveragePower(
      cycles, user_params.cycle_time, avg_power, avg_dynamic, avg_leak);
}

double HybridDatapath::getTotalMemArea() {
  return scratchpad->getTotalArea() + cache_area;
}

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
