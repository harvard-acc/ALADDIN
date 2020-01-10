#ifndef __HYBRID_DATAPATH_H__
#define __HYBRID_DATAPATH_H__

/* Hybrid datapath that can support arrays mapped to either scratchpads or
 * caches.
 */

#include <sys/time.h>
#include <deque>
#include <string>

#include "base/flags.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "dev/dma_device.hh"
#include "mem/mem_object.hh"
#include "mem/packet.hh"
#include "mem/request.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"
#include "sim/sim_exit.hh"
#include "sim/system.hh"

#include "aladdin/common/debugger/debugger_prompt.h"
#include "aladdin/common/DatabaseDeps.h"
#include "aladdin/common/ScratchpadDatapath.h"
#include "aladdin_tlb.hh"
#include "Gem5Datapath.h"
#include "MemoryQueue.h"

#include "debug/HybridDatapath.hh"
#include "params/HybridDatapath.hh"

class HybridDatapath : public ScratchpadDatapath, public Gem5Datapath {

 public:
  HybridDatapath(const HybridDatapathParams* params);
  ~HybridDatapath();

  bool queueCommand(std::unique_ptr<AcceleratorCommand> cmd) override;

  // Build, optimize, register and prepare datapath for scheduling.
  void initializeDatapath(int delay = 1) override;

  // Deletes all datapath state, including the TLB, but does not reset stats.
  virtual void clearDatapath();

  // Resets all stats.
  // NOTE: This is usually done through the Python configuration scripts.
  void resetCounters();

  // Start scheduling datapath
  void startDatapathScheduling(int delay = 1);

  /* Wrapper function for the real step() function to match EventWrapper
   * interface.
   */
  void eventStep();
  // Aladdin cycle step.
  bool step();
  // Handles how simulation exits.
  void exitSimulation();
  // Returns the tick event that will schedule the next step.
  Event& getTickEvent() override { return tickEvent; }

  // Attempts to schedule and execute all nodes in the executing queue.
  void stepExecutingQueue();

  // Register simulation statistics.
  void regStats();

  // Reset cache counters.
  void resetCacheCounters();

  // Notify the CPU that the accelerator is finished.
  void sendFinishedSignal() override;

  // Get the base address of the array (or array partition).
  Addr getBaseAddress(std::string label) override;

  // Insert a new virtual to physical address mapping into the Aladdin TLB.
  void insertTLBEntry(Addr vaddr, Addr paddr) override;

  // Insert an array label to its simulated virtual address mapping.
  void insertArrayLabelToVirtual(const std::string& array_label,
                                 Addr vaddr,
                                 size_t size) override;

  // Set the memory access type for an array.
  void setArrayMemoryType(const std::string& array_label,
                          MemoryType mem_type) override;

  void resetTrace() override;

  /* Invoked by the TLB when a TLB request has completed.
   *
   * If the translation did not miss, the the physical address is stored into
   * the memory queue. Otherwise, the status of the translation request is
   * reset to its original state while the miss is being processed.
   */
  void completeTLBRequest(PacketPtr pkt, bool was_miss);

  // Compute power and area of the caches.
  virtual double getTotalMemArea();
  virtual void getAverageMemPower(unsigned int cycles,
                                  float* avg_power,
                                  float* avg_dynamic,
                                  float* avg_leak);
  void getAverageSpadPower(unsigned int cycles,
                           float* avg_power,
                           float* avg_dynamic,
                           float* avg_leak);
  void getAverageCachePower(unsigned int cycles,
                            float* avg_power,
                            float* avg_dynamic,
                            float* avg_leak);
  void getAverageCacheQueuePower(unsigned int cycles,
                                 float* avg_power,
                                 float* avg_dynamic,
                                 float* avg_leak);
  void computeCactiResults();

 protected:
#ifdef USE_DB
  int writeConfiguration(sql::Connection* con);
#endif

  class DatapathSenderState : public Packet::SenderState {
   public:
    DatapathSenderState(bool _is_ctrl_signal, MemoryOpType _mem_type)
        : node_id(-1), vaddr(0), mem_type(_mem_type),
          is_ctrl_signal(_is_ctrl_signal) {}

    DatapathSenderState(unsigned _node_id, Addr _vaddr, MemoryOpType _mem_type)
        : node_id(_node_id), vaddr(_vaddr), mem_type(_mem_type),
          is_ctrl_signal(false) {}

    /* Aladdin node that triggered the memory access. */
    unsigned node_id;

    /* Virtual (trace) address for this memory access. */
    Addr vaddr;

    // The request type.
    MemoryOpType mem_type;

    /* Flag that determines whether a packet received on a data port is a
     * control signal accessed through memory (which needs to be handled
     * differently) or an ordinary memory access.
     */
    bool is_ctrl_signal;
  };

  class AladdinDmaEvent : public DmaEvent {
   private:
    /* To track which DMA request is returned. */
    unsigned dma_node_id;

   public:
    AladdinDmaEvent(HybridDatapath* datapath,
                    Addr startAddr,
                    unsigned _dma_node_id)
        : DmaEvent(datapath, startAddr), dma_node_id(_dma_node_id) {}
    AladdinDmaEvent(const AladdinDmaEvent& other)
        : DmaEvent(other.datapath, other.startAddr),
          dma_node_id(other.dma_node_id) {}
    /** Returns the description of the tick event. */
    const char* description() const override { return "AladdinDmaEvent"; }
    AladdinDmaEvent* clone() const override {
      return new AladdinDmaEvent(*this);
    }
    unsigned get_node_id() const { return dma_node_id; }
  };

  /* Error codes for attempts to issue a packet. */
  enum IssueResult {
    Accepted,       // Packet was accepted.
    DidNotAttempt,  // Port was already blocked, so could not issue.
    WillRetry,      // Port will retry this packet.
    NumIssueResultTypes,
  };

  // These callbacks will be called when the DMA (or cache, ACP) port receives
  // response (or retry).
  void dmaRespCallback(PacketPtr pkt) override;
  void cacheRespCallback(PacketPtr pkt) override;
  void cacheRetryCallback(PacketPtr pkt) override;
  void dmaCompleteCallback(DmaEvent* event) override;

  /* This translates a simulated virtual address (rather than a trace address)
   * to its physical address. However, we do this "invisibly" in zero time. This
   * is because DMAs are typically initiated by the CPU using physical
   * addresses, but our DMA model initiates transfers from the accelerator,
   * which can't know physical addresses without a translation.
   */
  Addr translateAtomic(Addr vaddr, int size) override;

  /* Returns the memory op type for this node (or array label).
   *
   * The memory op type depends on the datapath's particular configuration,
   * so this must be determined by the datapath.
   */
  MemoryOpType getMemoryOpType(ExecNode* node);
  MemoryOpType getMemoryOpType(const std::string& array_label);

  /* Handle a register memory operation and return true if success.
   *
   * Registers currently have no bandwidth limits, so this will always
   * succeed.
   */
  bool handleRegisterMemoryOp(ExecNode* node);

  /* Handle a scratchpad memory operation and return true if satisfied.
   *
   * Scratchpads do have a maximum number of ports. A memory operation will
   * only be satisfied if there is sufficient bandwidth.
   */
  bool handleSpadMemoryOp(ExecNode* node);

  /* Handle a DMA request and return true if the DMA request has returned.
   *
   * Since DMA requests are multicycle operations, this only returns true
   * when the request has been completed by the system.
   *
   * This function handles the various behavorial stages of a DMA request.
   * The mechanics of actually performing the access are delegated to worker
   * functions.
   */
  bool handleDmaMemoryOp(ExecNode* node);

  /* Handle an access to ready bits on the scratchpad.
   *
   * Ready bit accesses are currently modeled as a single cycle operation.
   */
  bool handleReadyBitAccess(ExecNode* node);

  /* Handle a cache memory access and return true if the data has returned.
   *
   * Currently, address translation and cache accesses are serialized
   * operations. They do not occur in parallel.
   *
   * This function handles the various behavorial stages of a cache access.
   * The mechanics of actually performing the access are delegated to worker
   * functions.
   */
  bool handleCacheMemoryOp(ExecNode* node);

  /* Handle an ACP memory access and return true if the data has returned.
   *
   * ACP requests share the same cache queue as local cache requests, but
   * unlike local cache requests, there is no translation latency since ACP
   * uses physical addresses.
   *
   * This function handles the various behavorial stages of an ACP access.
   * The mechanics of actually performing the access are delegated to worker
   * functions.
   */
  bool handleAcpMemoryOp(ExecNode* node);

  // If the specified vaddr doesn't match any entry in the cache queue, create
  // an entry for it and insert it to the cache queue.
  MemoryQueueEntry* createAndQueueAcpEntry(Addr vaddr,
                                           int size,
                                           bool isLoad,
                                           const std::string& array_label,
                                           unsigned node_id);
  // Check and update the ACP entry's status and take actions correspondingly.
  bool updateAcpEntryStatus(MemoryQueueEntry* entry, ExecNode* node);

  /* Prints the ids of all nodes currently on the executing queue.
   *
   * Useful for debugging deadlocks.
   */
  void printExecutingQueue();

  /* Delete all memory queue entries that have returned. */
  void retireReturnedMemQEntries();

  // Wrapper for initializeDatapath() to match EventWrapper interface.
  //

  void enterDebuggerIfEnabled() {
    using namespace adb;
    if (!use_aladdin_debugger)
      return;

    HandlerRet ret = interactive_mode(static_cast<ScratchpadDatapath*>(this));
    if (ret == QUIT)
      exitSimLoop("User exited from debugger.");
  }

  // DMA access functions.
  void delayedDmaIssue();  // Used to postpone a call to issueDmaRequest().
  void issueDmaRequest(unsigned node_id);
  void addDmaNodeToIssueQueue(unsigned node_id);
  void incrementDmaScratchpadAccesses(std::string array_label,
                                      unsigned dma_size,
                                      bool is_dma_load);

  // Issue a virtual address translation timing request.
  bool issueTLBRequestTiming(Addr addr,
                             unsigned size,
                             bool isLoad,
                             SrcTypes::Variable* var);

  // Return the address translation of this trace address immediately.
  //
  // Sometimes, we don't want to pay the latency cost of a TLB access. For
  // example, DMA/ACP accesses assume the use of physical addresses, but the
  // trace has no way of getting physical addresses, so a zero-latency TLB access
  // must be used to model this behavior.
  //
  // Returns:
  //   An AladdinTLBResponse object, containing the the simulation's vaddr and
  //   paddr corresponding to this trace address.
  AladdinTLBResponse getAddressTranslation(Addr trace_addr,
                                           unsigned size,
                                           bool isLoad,
                                           SrcTypes::Variable* var);

  // Create a TLB request packet.
  //
  // The Variable pointer is meant to contain the name of the array being
  // accessed, which is not necessarily the same as the Variable object
  // attached to the current node (that Variable would refer to the
  // register/variable that contains the actual address).
  PacketPtr createTLBRequestPacket(Addr addr,
                                   unsigned size,
                                   bool isLoad,
                                   SrcTypes::Variable* var);

  // Cache/ACP access functions.
  IssueResult issueCacheRequest(Addr vaddr,
                                Addr paddr,
                                unsigned size,
                                bool isLoad,
                                unsigned node_id,
                                uint8_t* data);

  IssueResult issueAcpRequest(Addr vaddr,
                              Addr paddr,
                              unsigned size,
                              bool isLoad,
                              unsigned node_id,
                              uint8_t* data);

  // A helper function for issuing either cache or ACP requests.
  IssueResult issueCacheOrAcpRequest(MemoryOpType op_type,
                                     Addr vaddr,
                                     Addr paddr,
                                     unsigned size,
                                     bool isLoad,
                                     unsigned node_id,
                                     uint8_t* data);

  // For ACP: Request ownership of a cache line.
  IssueResult issueOwnershipRequest(Addr vaddr,
                                    Addr paddr,
                                    unsigned size,
                                    unsigned node_id);

  // Update the status of a cache request after receiving a response.
  //
  // This terminates the lifetime of the packet and response.
  void updateCacheRequestStatusOnResp(PacketPtr pkt,
                                      DatapathSenderState* state);

  // Update the status of a retried cache request.
  //
  // This does not deallocate the packet, since all we know is that the
  // packet has been successfully sent, but the receiver may opt to reuse the
  // packet object for the response.
  void updateCacheRequestStatusOnRetry(PacketPtr pkt);

  Addr toCacheLineAddr(Addr addr) {
    return (addr / cacheLineSize) * cacheLineSize;
  }

  // Marks a node as started and increments a stats counter.
  virtual void markNodeStarted(ExecNode* node);

  // This is used to measure the elapsed time on the host machine.
  timeval beginTime;

  // True if the accelerator is busy.
  bool busy;

  // True if the accelerator has received the acknowledgement of the sent finish
  // signal.
  bool ackedFinishSignal;

  // Command queue for incoming commands from CPUs.
  std::deque<std::unique_ptr<AcceleratorCommand>> commandQueue;

  /* Stores status information about memory nodes currently in flight.
   *
   * TODO: DMA memory ops still use this map, but for all other memory
   * operations, this is not used. Get rid of this entirely, including for
   * DMA ops.
   */
  std::map<unsigned, MemAccessStatus> inflight_dma_nodes;

  // A bursty memory access node will be broken up into cacheline size requests.
  // This tracks the outstanding requests of the bursty nodes (except DMA).
  // TODO: Refactor the host ACP/cache accesses into the ACP/Cache ports, so we
  // don't need to track them here.
  std::map<unsigned, std::list<MemoryQueueEntry*>> inflight_burst_nodes;

  /* Hash table to track DMA accesses. Indexed by the base address of DMA
   * accesses, and mapped to the corresponding node id for that DMA request. */
  // std::unordered_map<Addr, unsigned> dmaIssueQueue;
  std::set<unsigned> dmaIssueQueue;

  // This queue stores the DMA events that are waiting for their setup latency
  // to elapse before they can be issued, along with their setup latencies.
  std::deque<std::pair<unsigned, Tick>> dmaWaitingQueue;

  // Number of cycles required for the CPU to initiate a DMA transfer.
  unsigned dmaSetupOverhead;

  // Tracks outstanding cache access nodes.
  MemoryQueue cache_queue;

  /* NOTE: These are provided by gem5's caches directly but I have no idea how
   * to access them. Based on an extensive Google search, there doesn't seem
   * like a good answer even exists - all the power modeling appears to be done
   * after the fact rather than during runtime. For now, I'm just going to
   * track total loads and stores, regardless of whether they hit or miss.
   * Simulations confirm that they agree with GEM5's cache stats.
   */
  Stats::Scalar dcache_loads;
  Stats::Scalar dcache_stores;
  Stats::Histogram dcache_latency;
  Stats::Histogram dcache_queue_time;

  // ACP related statistics.
  Stats::Scalar acp_loads;
  Stats::Scalar acp_stores;
  Stats::Histogram acp_latency;
  Stats::Histogram acp_queue_time;

  // Total cycles spent on DMA setup.
  Stats::Scalar dma_setup_cycles;

  // Total accelerator cycles, accumulated across invocations unless reset.
  //
  // Although Aladdin will print this anyways, registering the stat will make
  // it show up in stats.txt.
  Stats::Scalar acc_sim_cycles;

  // If True, this exits the sim loop at the end of each accelerator
  // invocation so that stats can be dumped. The simulation script will
  // resume execution afterwards.
  const bool enable_stats_dump;

  // ACP has been enabled in this system. If this flag is false, Aladdin will
  // terminate simulation if an ACP access is attempted.
  const bool acp_enabled;

  // Enable or disable the ACP L1 cache implementation method.
  //
  // There are currently two implemented ways to handle ACP coherency logic:
  //   1. Directly connect the ACP port to the L2 cache crossbar. We will issue
  //      all the cache coherency commands ourselves.
  //
  //   2. Place a tiny L1 cache (just one or two cache lines large) in between
  //      the ACP port and the L2 cache crossbar and just issue reads and writes
  //      out of the port. The cache will handle all coherency traffic for us.
  //      The cache parameters must be carefully tuned so that latency-wise, it
  //      looks like we're still directly accessing L2.
  //
  // If this value is true, we use method 2; otherwise, we use method 1.
  const bool use_acp_cache;

  // Data cache structural parameters.
  std::string cacheSize;  // Because of GEM5's stupid xxkB format.
  std::string cacti_cfg;  // CACTI configuration file for the main cache.
  unsigned cacheLineSize;
  unsigned cacheHitLatency;
  unsigned cacheAssoc;

  // TODO: These parameters are for the DMA model and should find a new home
  // eventually.  If we switch to Ruby instead of the classic cache model then
  // we can actually issue cache flush requests and really model what's
  // happening instead of just accounting for latency.
  unsigned cacheLineFlushLatency;
  unsigned cacheLineInvalidateLatency;

  // Cache energy and area numbers generated by CACTI.
  float cache_readEnergy;
  float cache_writeEnergy;
  float cache_leakagePower;
  float cache_area;

  // Accelerator TLB implementation.
  AladdinTLB dtb;

  // If true, then successive DMA nodes are pipelined to overlap transferring
  // data of one DMA transaction with the setup latency of the next. If false,
  // then DMA nodes wait for all transactions to finish setup before issuing.
  const bool pipelinedDma;

  // Skip the initial delay caused by flushing cache lines on DMA.
  const bool ignoreCacheFlush;

  // Start the Aladdin debugger.
  const bool use_aladdin_debugger;

  // gem5 tick
  EventWrapper<HybridDatapath, &HybridDatapath::eventStep> tickEvent;
  // Delayed DMA issue
  EventWrapper<HybridDatapath, &HybridDatapath::delayedDmaIssue> delayedDmaEvent;
  // Datapath re-initialization.
  //
  // Event to start the debugger's interactive mode.
  EventWrapper<HybridDatapath, &HybridDatapath::enterDebuggerIfEnabled>
      enterDebuggerEvent;

  // Number of executed nodes as of the last event trigger.
  // This is required for deadlock detection.
  unsigned executedNodesLastTrigger;

  // Name of the datapath object assigned by gem5.
  std::string datapath_name;
};

#endif
