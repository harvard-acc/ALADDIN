#ifndef __HYBRID_DATAPATH_H__
#define __HYBRID_DATAPATH_H__

/* Hybrid datapath that can support arrays mapped to either scratchpads or
 * caches.
 */

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

  // Build, optimize, register and prepare datapath for scheduling.
  virtual void initializeDatapath(int delay = 1);
  // Start scheduling datapath
  void startDatapathScheduling(int delay = 1);
  virtual void clearDatapath();
  void resetCounters(bool flush_tlb);
  void clearDatapath(bool flush_tlb);

  virtual MasterPort& getDataPort() {
    return spadPort;
  };
  virtual MasterPort& getCachePort() {
    return cachePort;
  };
  MasterID getSpadMasterId() {
    return spadMasterId;
  };
  MasterID getCacheMasterId() {
    return cacheMasterId;
  };

  /**
   * Get a master port on this CPU. All CPUs have a data and
   * instruction port, and this method uses getDataPort and
   * getInstPort of the subclasses to resolve the two ports.
   *
   * @param if_name the port name
   * @param idx ignored index
   *
   * @return a reference to the port with the given name
   */
  BaseMasterPort& getMasterPort(const std::string& if_name,
                                PortID idx = InvalidPortID);

  /* Wrapper function for the real step() function to match EventWrapper
   * interface.
   */
  void eventStep();
  // Aladdin cycle step.
  bool step();
  // Handles how simulation exits.
  void exitSimulation();
  // Returns the tick event that will schedule the next step.
  Event& getTickEvent() { return tickEvent; }

  // Attempts to schedule and execute all nodes in the executing queue.
  void stepExecutingQueue();

  // Register simulation statistics.
  void regStats();

  // Reset cache counters.
  void resetCacheCounters();

  // Notify the CPU that the accelerator is finished.
  virtual void sendFinishedSignal();

  // Get the base address of the array (or array partition).
  virtual Addr getBaseAddress(std::string label);

  // Insert a new virtual to physical address mapping into the Aladdin TLB.
  virtual void insertTLBEntry(Addr vaddr, Addr paddr);

  // Insert an array label to its simulated virtual address mapping.
  virtual void insertArrayLabelToVirtual(std::string array_label, Addr vaddr);

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

 private:
  // typedef uint32_t FlagsType;

  /* All possible types of memory operations supported by Aladdin. */
  enum MemoryOpType {
    Register,
    Scratchpad,
    Cache,
    Dma
  };

  /* Returns the memory op type for this node.
   *
   * The memory op type depends on the datapath's particular configuration,
   * so this must be determined by the datapath.
   */
  MemoryOpType getMemoryOpType(ExecNode* node);

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

  /* Prints the ids of all nodes currently on the executing queue.
   *
   * Useful for debugging deadlocks.
   */
  void printExecutingQueue();

  /* Delete all memory queue entries that have returned. */
  void retireReturnedMemQEntries();

  // Wrapper for initializeDatapath() to match EventWrapper interface.
  void reinitializeDatapath() { initializeDatapath(1); }

  // DMA access functions.
  void delayedDmaIssue();  // Used to postpone a call to issueDmaRequest().
  void issueDmaRequest(unsigned node_id);
  void completeDmaRequest(unsigned node_id);
  void addDmaNodeToIssueQueue(unsigned node_id);
  void incrementDmaScratchpadAccesses(std::string array_label,
                                      unsigned dma_size,
                                      bool is_dma_load);

  // Virtual address translation.
  bool issueTLBRequestTiming(Addr addr, unsigned size, bool isLoad, unsigned node_id);
  bool issueTLBRequestInvisibly(Addr addr, unsigned size, bool isLoad, unsigned node_id);
  PacketPtr createTLBRequestPacket(Addr addr, unsigned size, bool isLoad, unsigned node_id);

  // Cache access functions.
  bool issueCacheRequest(Addr addr,
                         unsigned size,
                         bool isLoad,
                         unsigned node_id,
                         uint64_t value);
  void completeCacheRequest(PacketPtr pkt);

  // Marks a node as started and increments a stats counter.
  virtual void markNodeStarted(ExecNode* node);

  /* This port has to accept snoops but it doesn't need to do anything. See
   * arch/arm/table_walker.hh
   */
  class SpadPort : public DmaPort {
   public:
    SpadPort(HybridDatapath* dev, System* s, unsigned _max_req,
             unsigned _chunk_size, unsigned _numChannels,
             bool _invalidateOnDmaStore)
        : DmaPort(dev, s, _max_req, _chunk_size,
                  _numChannels, _invalidateOnDmaStore),
          max_req(_max_req), datapath(dev) {}
    // Maximum DMA requests that can be queued.
    const unsigned max_req;

   protected:
    virtual bool recvTimingResp(PacketPtr pkt);
    virtual void recvTimingSnoopReq(PacketPtr pkt) {}
    virtual void recvFunctionalSnoop(PacketPtr pkt) {}
    virtual bool isSnooping() const { return true; }
    HybridDatapath* datapath;
  };

  // Port for cache coherent memory accesses. This implementation does not
  // support functional or atomic accesses.
  class CachePort : public MasterPort {
   public:
    CachePort(HybridDatapath* dev)
        : MasterPort(dev->name() + ".cache_port", dev), datapath(dev) {}

   protected:
    virtual bool recvTimingResp(PacketPtr pkt);
    virtual void recvTimingSnoopReq(PacketPtr pkt) {}
    virtual void recvFunctionalSnoop(PacketPtr pkt) {}
    virtual Tick recvAtomicSnoop(PacketPtr pkt) { return 0; }
    virtual void recvReqRetry();
    virtual void recvRespRetry() {}
    virtual bool isSnooping() const { return true; }
    HybridDatapath* datapath;
  };

  class DatapathSenderState : public Packet::SenderState {
   public:
    DatapathSenderState(unsigned _node_id, bool _is_ctrl_signal = false)
        : node_id(_node_id), is_ctrl_signal(_is_ctrl_signal) {}

    /* Aladdin node that triggered the memory access. */
    unsigned node_id;

    /* Flag that determines whether a packet received on a data port is a
     * control signal accessed through memory (which needs to be handled
     * differently) or an ordinary memory access.
     */
    bool is_ctrl_signal;
  };

  class DmaEvent : public Event {
   private:
    HybridDatapath* datapath;
    /* To track which DMA request is returned. */
    unsigned dma_node_id;

   public:
    /** Constructor */
    DmaEvent(HybridDatapath* _dpath, unsigned _dma_node_id);
    /** Process a dma event */
    void process();
    /** Returns the description of the tick event. */
    const char* description() const;
    unsigned get_node_id() { return dma_node_id; }
  };

  /* Stores status information about memory nodes currently in flight.
   *
   * TODO: DMA memory ops still use this map, but for all other memory
   * operations, this is not used. Get rid of this entirely, including for
   * DMA ops.
   */
  std::map<unsigned, MemAccessStatus> inflight_dma_nodes;

  /* Hash table to track DMA accesses. Indexed by the base address of DMA
   * accesses, and mapped to the corresponding node id for that DMA request. */
  // std::unordered_map<Addr, unsigned> dmaIssueQueue;
  std::set<unsigned> dmaIssueQueue;

  // This queue stores the DMA events that are waiting for their setup latency
  // to elapse before they can be issued, along with their setup latencies.
  std::deque<std::pair<unsigned, Tick>> dmaWaitingQueue;

  // Number of cycles required for the CPU to initiate a DMA transfer.
  unsigned dmaSetupOverhead;

  // DMA port to the scratchpad.
  SpadPort spadPort;
  MasterID spadMasterId;

  // Coherent port to the cache.
  CachePort cachePort;
  MasterID cacheMasterId;

  // Tracks outstanding cache access nodes.
  MemoryQueue cache_queue;

  /* NOTE: These are provided by gem5's caches directly but I have no idea how
   * to access them. Based on an extensive Google search, there doesn't seem
   * like a good answer even exists - all the power modeling appears to be done
   * after the fact rather than during runtime. For now, I'm just going to
   * track total loads and stores, regardless of whether they hit or miss.
   * Simulations confirm that they agree with GEM5's cache stats.
   */
  Stats::Scalar loads;
  Stats::Scalar stores;

  // Total cycles spent on DMA setup.
  Stats::Scalar dma_setup_cycles;

  // Total accelerator cycles, accumulated across invocations unless reset.
  //
  // Although Aladdin will print this anyways, registering the stat will make
  // it show up in stats.txt.
  Stats::Scalar acc_sim_cycles;

  // True if the cache's MSHRs are full.
  bool isCacheBlocked;

  // If True, this exits the sim loop at the end of each accelerator
  // invocation so that stats can be dumped. The simulation script will
  // resume execution afterwards.
  bool enable_stats_dump;

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

  // Retry packet for cache accesses.
  PacketPtr retryPkt;

  // If true, then successive DMA nodes are pipelined to overlap transferring
  // data of one DMA transaction with the setup latency of the next. If false,
  // then DMA nodes wait for all transactions to finish setup before issuing.
  bool pipelinedDma;

  bool ignoreCacheFlush;

  // gem5 tick
  EventWrapper<HybridDatapath, &HybridDatapath::eventStep> tickEvent;
  // Delayed DMA issue
  EventWrapper<HybridDatapath, &HybridDatapath::delayedDmaIssue> delayedDmaEvent;
  // Datapath re-initialization.
  //
  // Used in standalone mode to support multiple invocations.
  EventWrapper<HybridDatapath, &HybridDatapath::reinitializeDatapath> reinitializeEvent;

  // Number of executed nodes as of the last event trigger.
  // This is required for deadlock detection.
  unsigned executedNodesLastTrigger;

  // Name of the datapath object assigned by gem5.
  std::string datapath_name;
};

#endif
