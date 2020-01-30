#ifndef __GEM5_DATAPATH_H__
#define __GEM5_DATAPATH_H__

#include "dev/dma_device.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"
#include "sim/system.hh"

#include "aladdin_sys_connection.h"

#include "debug/Gem5Datapath.hh"
#include "debug/Gem5DatapathVerbose.hh"

class AcceleratorCommand;

/* A collection of functions common to datapath objects used within GEM5.
 * Note: to avoid potential problems arising from the use of multiple
 * inheritance with this integration, Gem5Datapath should never extend
 * BaseDatapath.
 */
class Gem5Datapath : public ClockedObject {

 public:
  /* These extra parameters are needed because making a Gem5DatapathParams
   * object that serves as a base class to HybridDatapathParams.
   * Gem5Datapath has to pass the base class of MemObjectParams to its parent
   * constructor, which doesn't contain any of our custom parameters.
   */
  Gem5Datapath(const Params* params,
               int _accelerator_id,
               int maxDmaRequests,
               int dmaChunkSize,
               int numDmaChannels,
               bool invalidateOnDmaStore,
               System* _system)
      : ClockedObject(params), spadPort(this,
                                        _system,
                                        maxDmaRequests,
                                        dmaChunkSize,
                                        numDmaChannels,
                                        invalidateOnDmaStore),
        spadMasterId(_system->getMasterId(this, name() + ".spad")),
        cachePort(this, "cache_port"),
        cacheMasterId(_system->getMasterId(this, name() + ".cache")),
        acpPort(this, "acp_port"),
        acpMasterId(_system->getMasterId(this, name() + ".acp")),
        accelerator_id(_accelerator_id), finish_flag(0), context_id(-1),
        thread_id(-1), cycles_since_last_node(0), system(_system) {}

  Port& getPort(const std::string& if_name,
                PortID idx = InvalidPortID) override {
    if (if_name == "spad_port")
      return spadPort;
    else if (if_name == "cache_port")
      return cachePort;
    else if (if_name == "acp_port")
      return acpPort;
    else
      return ClockedObject::getPort(if_name);
  }

  // Insert a command to the accelerator.
  virtual bool queueCommand(std::unique_ptr<AcceleratorCommand> cmd) = 0;

  /* Return the tick event object for the event queue for this datapath. */
  virtual Event& getTickEvent() = 0;

  /* Build, optimize, register and prepare datapath for scheduling. */
  virtual void initializeDatapath(int delay = 1) = 0;

  /* Add the tick event to the gem5 event queue. */
  void scheduleOnEventQueue(unsigned delay_cycles = 1) {
    schedule(getTickEvent(), clockEdge(Cycles(delay_cycles)));
  }

  /* Set the accelerator's context ids to be that of the invoking thread. */
  void setContextThreadIds(int _context_id, int _thread_id) {
    context_id = _context_id;
    thread_id = _thread_id;
  }

  void setFinishFlag(Addr _finish_flag) { finish_flag = _finish_flag; }

  /* Use this to set the accelerator parameters. */
  virtual void setParams(std::unique_ptr<uint8_t[]> accel_params) {}

  /* Send a signal to the rest of the system that the accelerator has
   * finished. This signal takes the form of a shared memory block. This is
   * used by user level programs that invoke Aladdin during program execution
   * and spin while waiting for the accelerator to complete.  Different
   * datapath implementations will need different mechanisms to accomplish
   * this.
   */
  virtual void sendFinishedSignal() = 0;

  /* Wake up the CPU thread that invokes the datapath. */
  virtual void wakeupCpuThread() {
    ThreadContext* cpuThread = system->getThreadContext(context_id);
    cpuThread->activate();
  }

  /* Return the base address of the array specified by label. The base address
   * corresponds to the zeroth element of the array.
   */
  virtual Addr getBaseAddress(std::string label) = 0;

  /* Insert a vpn -> ppn mapping into the datapath's TLB.
   *
   * This is currently used to create a mapping between the simulated virtual
   * address of an array in the program with the actual physical address in
   * the simulator.
   *
   * Datapath models that do not support TLB structures should abort
   * simulation if this is called.
   *
   * TODO(samxi): Maybe this should not be part of Gem5Datapath? But I'm not
   * sure if a dynamic_cast to HybridDatapath would work here.
   */
  virtual void insertTLBEntry(Addr vaddr, Addr paddr) = 0;

  /* Insert a mapping between array labels to their simulated virtual
   * addresses. */
  virtual void insertArrayLabelToVirtual(const std::string& array_label,
                                         Addr vaddr,
                                         size_t size) = 0;

  // Set the memory access type for an array.
  virtual void setArrayMemoryType(const std::string& array_label,
                                  MemoryType mem_type){};

  /* Reset the dynamic trace to the beginning. */
  virtual void resetTrace() = 0;

  int getContextId() const { return context_id; }
  int getThreadId() const { return thread_id; }

 protected:
  class DmaEvent : public Event {
   protected:
    Gem5Datapath* datapath;
    // This is the original simulated start virtual address of the DMA request
    // from the datapath.
    Addr startAddr;
    // This is the virtual address offset of a DMA request with respect to the
    // startAddr above.
    Addr reqOffset;

   public:
    DmaEvent(Gem5Datapath* _datapath, Addr _startAddr)
        : Event(Default_Pri, AutoDelete), datapath(_datapath),
          startAddr(_startAddr) {}
    DmaEvent(const DmaEvent& other)
        : Event(Default_Pri, AutoDelete), datapath(other.datapath),
          startAddr(other.startAddr) {}
    void process() override {
      // If all the outstanding requests of this DMA request have responded,
      // inform the datapath.
      if (--datapath->spadPort.outstandingReqs[startAddr] == 0) {
        datapath->dmaCompleteCallback(this);
        datapath->spadPort.outstandingReqs.erase(startAddr);
      }
    }
    const char* description() const override { return "DmaEvent"; }
    virtual DmaEvent* clone() const { return new DmaEvent(*this); }
    Addr getStartAddr() const { return startAddr; }
    void setReqOffset(Addr offset) { reqOffset = offset; }
    Addr getReqOffset() const { return reqOffset; }
  };

  /* This port has to accept snoops but it doesn't need to do anything. See
   * arch/arm/table_walker.hh
   */
  class SpadPort : public DmaPort {
   public:
    SpadPort(Gem5Datapath* dev,
             System* s,
             unsigned _max_req,
             unsigned _chunk_size,
             unsigned _numChannels,
             bool _invalidateOnDmaStore)
        : DmaPort(dev,
                  s,
                  _max_req,
                  _chunk_size,
                  _numChannels,
                  _invalidateOnDmaStore),
          datapath(dev) {}

   protected:
    virtual bool recvTimingResp(PacketPtr pkt) {
      if (pkt->cmd == MemCmd::InvalidateResp) {
        // TODO: We can create a completion event for invalidation responses so
        // that DMA nodes cannot proceed until the invalidations are done.
        return DmaPort::recvTimingResp(pkt);
      }
      datapath->dmaRespCallback(pkt);
      return DmaPort::recvTimingResp(pkt);
    }
    virtual void recvTimingSnoopReq(PacketPtr pkt) {}
    virtual void recvFunctionalSnoop(PacketPtr pkt) {}
    virtual bool isSnooping() const { return true; }
    Gem5Datapath* datapath;
    // This tracks the original request (the start vaddr) and its outstanding
    // sub-requests.
    std::unordered_map<Addr, int> outstandingReqs;
    friend class DmaEvent;
    friend class Gem5Datapath;
  };

  // Port for cache coherent memory accesses. This implementation does not
  // support functional or atomic accesses.
  class CachePort : public MasterPort {
   public:
    CachePort(Gem5Datapath* dev, const std::string& name)
        : MasterPort(dev->name() + "." + name, dev), datapath(dev),
          retryPkt(nullptr) {}

    bool inRetry() const { return retryPkt != NULL; }
    void setRetryPkt(PacketPtr pkt) { retryPkt = pkt; }
    void clearRetryPkt() { retryPkt = NULL; }
   protected:
    bool recvTimingResp(PacketPtr pkt) override {
      DPRINTF(Gem5Datapath, "%s: for address: %#x %s\n", __func__,
              pkt->getAddr(), pkt->cmdString());
      if (pkt->isError()) {
        DPRINTF(Gem5Datapath,
                "Got error packet back for address: %#x\n",
                pkt->getAddr());
      }
      datapath->cacheRespCallback(pkt);
      delete pkt->popSenderState();
      delete pkt;
      return true;
    }
    void recvTimingSnoopReq(PacketPtr pkt) override {}
    void recvFunctionalSnoop(PacketPtr pkt) override {}
    Tick recvAtomicSnoop(PacketPtr pkt) override { return 0; }
    void recvReqRetry() override {
      assert(inRetry());
      assert(retryPkt->isRequest());
      DPRINTF(
          Gem5Datapath, "recvReqRetry for paddr: %#x \n", retryPkt->getAddr());
      if (sendTimingReq(retryPkt)) {
        DPRINTF(Gem5DatapathVerbose, "Retry pass!\n");
        datapath->cacheRetryCallback(retryPkt);
        clearRetryPkt();
      } else {
        DPRINTF(Gem5DatapathVerbose, "Still blocked!\n");
      }
    }
    bool isSnooping() const override { return true; }

    Gem5Datapath* datapath;
    PacketPtr retryPkt;
  };

  // ACP is connected to the L2 cache, but since there is no other caching
  // agent to which it is attached (unlike the CachePort), it cannot accept
  // snoops.
  class AcpPort : public CachePort {
    // Inherit parent constructors.
    using CachePort::CachePort;

   protected:
    // The ACP port does not snoop transactions. If it did, then it could be
    // considered the "holder" of cache lines, when in fact the L2 is still the
    // holder.
    virtual bool isSnooping() const { return false; }
  };

  MasterID getSpadMasterId() const { return spadMasterId; }
  MasterID getCacheMasterId() const { return cacheMasterId; }
  MasterID getAcpMasterId() const { return acpMasterId; }

  // Callback functions to the datapath.
  virtual void dmaRespCallback(PacketPtr pkt) {}
  virtual void cacheRespCallback(PacketPtr pkt) {}
  virtual void cacheRetryCallback(PacketPtr pkt) {}
  // The callback when the complete DMA request has finished.
  virtual void dmaCompleteCallback(DmaEvent* event) {}

  // Address translation function, it returns the translation result without
  // accounting for timing.
  virtual Addr translateAtomic(Addr vaddr, int size) = 0;

  // Send an DMA request. This splits the request if it crosses page boundaries,
  // translates it, and then sends it to the DMA controller.
  // TODO: Make the address translation account for timing, which would require
  // a state machine.
  virtual void splitAndSendDmaRequest(
      Addr startAddr, int size, bool isRead, uint8_t* data, DmaEvent* event) {
    // Initialize the outstanding page-aligned requests for this DMA request.
    // This assumes no subsequent DMA request will be issued from the datapath
    // if an outstanding one accesses the same start address.
    assert(spadPort.outstandingReqs.find(startAddr) ==
               spadPort.outstandingReqs.end() &&
           "There is an inflight DMA that accesses the same start address!");
    spadPort.outstandingReqs[startAddr] = 0;

    Addr firstPageVaddr = startAddr & ~pageMask();
    int firstPageOffset = startAddr & pageMask();

    int remainingBytes = size;
    int i = 0;
    do {
      Addr dmaReqVaddr = i == 0 ? startAddr : firstPageVaddr + i * pageBytes();
      int dmaReqSize =
          i == 0 ? std::min(remainingBytes, pageBytes() - firstPageOffset)
                 : std::min(remainingBytes, pageBytes());
      // When we break the original DMA request up into page-sized ones, we need
      // to create a different DMA event for every sub-request because 1) the
      // DMA event will be auto-deleted once the first sub-request finishes and
      // schedules it, though it can be worked around by turning off the
      // auto-delete feature of gem5 events. 2) the DMA event tracks offset
      // information of a sub-request.
      if (i > 0)
        event = event->clone();
      // Set the DMA request offset.
      event->setReqOffset(dmaReqVaddr - startAddr);
      i++;
      remainingBytes -= dmaReqSize;

      // Do the address translation.
      Addr dmaReqPaddr;
      dmaReqPaddr = translateAtomic(dmaReqVaddr, dmaReqSize);

      // Prepare the DMA transaction.
      MemCmd::Command cmd = isRead ? MemCmd::ReadReq : MemCmd::WriteReq;
      // Marking the DMA packets as uncacheable ensures they are not snooped by
      // caches.
      Request::Flags flags = Request::UNCACHEABLE;

      DPRINTF(Gem5DatapathVerbose,
              "Sending DMA %s for paddr %#x with size %d.\n",
              isRead ? "read" : "write",
              dmaReqPaddr,
              dmaReqSize);
      spadPort.dmaAction(cmd, dmaReqPaddr, dmaReqSize, event, data, 0, flags);
      spadPort.outstandingReqs[startAddr]++;
      data += dmaReqSize;
    } while (remainingBytes > 0);
  }

  Addr pageMask() { return system->getPageBytes() - 1; }
  int pageBytes() { return system->getPageBytes(); }

  // DMA port to the scratchpad.
  SpadPort spadPort;
  MasterID spadMasterId;

  // Coherent port to the cache.
  CachePort cachePort;
  MasterID cacheMasterId;

  // ACP port to the system's L2 cache.
  AcpPort acpPort;
  MasterID acpMasterId;

  /* Accelerator id, assigned by the system. It can also be the ioctl request
   * code for the particular kernel.
   */
  int accelerator_id;

  /* The accelerator will call sendFinishedSignal on this address to signal
   * its completion to the CPU. This is a physical address.
   */
  Addr finish_flag;

  /* Thread and context ids of the thread that activated this accelerator. */
  int context_id;
  int thread_id;

  /* Deadlock detector. Counts the number of ticks that have passed since the
   * last node was executed. If this exceeds deadlock_threshold, then
   * simulation is terminated with an error. */
  unsigned cycles_since_last_node;
  const unsigned deadlock_threshold = 1000000;

  /* Pointer to the rest of the system. */
  System* system;
};

class AcceleratorCommand {
 public:
  AcceleratorCommand() {}
  virtual ~AcceleratorCommand() {}

  // Execute the command.
  virtual void run(Gem5Datapath* accel) = 0;

  // Return the name of the command.
  virtual std::string name() const = 0;

  // True if this is a blocking command (no subsequent commands can run until it
  // finishes).
  virtual bool blocking() const { return true; };
};

class ActivateAcceleratorCmd : public AcceleratorCommand {
 public:
  ActivateAcceleratorCmd(Addr _finishFlag,
                         std::unique_ptr<uint8_t[]> _accelParams,
                         int _contextId,
                         int _threadId,
                         int _delay)
      : AcceleratorCommand(), finishFlag(_finishFlag),
        accelParams(std::move(_accelParams)), contextId(_contextId),
        threadId(_threadId), delay(_delay) {}

  void run(Gem5Datapath* accel) override {
    /* Register a pointer to use for communication between accelerator and
     * CPU.
     */
    accel->setFinishFlag(finishFlag);
    /* Sets context and thread ids for a given accelerator. These are needed
     * for supporting cache prefetchers.
     */
    accel->setContextThreadIds(contextId, threadId);
    /* Set the accelerator params. */
    accel->setParams(std::move(accelParams));
    /* Adds the specified accelerator to the event queue with a given number
     * of delay cycles (to emulate software overhead during invocation).
     */
    accel->initializeDatapath(delay);
  }

  std::string name() const override { return "ActivateAccelerator command"; }

 protected:
  Addr finishFlag;
  std::unique_ptr<uint8_t[]> accelParams;
  int contextId;
  int threadId;
  int delay;
};

class InsertArrayLabelMappingCmd : public AcceleratorCommand {
 public:
  InsertArrayLabelMappingCmd(Addr _simVaddr, Addr _simPaddr)
      : AcceleratorCommand(), simVaddr(_simVaddr), simPaddr(_simPaddr) {}

  void run(Gem5Datapath* accel) override {
    accel->insertTLBEntry(simVaddr, simPaddr);
  }

  std::string name() const override {
    return "InsertArrayLabelMapping command";
  }

  bool blocking() const override { return false; }

 protected:
  Addr simVaddr;
  Addr simPaddr;
};

class InsertAddressTranslationMappingCmd : public AcceleratorCommand {
 public:
  InsertAddressTranslationMappingCmd(const std::string& _arrayLabel,
                                     Addr _simVaddr,
                                     size_t _size)
      : AcceleratorCommand(), arrayLabel(_arrayLabel), simVaddr(_simVaddr),
        size(_size) {}

  void run(Gem5Datapath* accel) override {
    accel->insertArrayLabelToVirtual(arrayLabel, simVaddr, size);
  }

  std::string name() const override {
    return "InsertAddressTranslationMapping command";
  }

  bool blocking() const override { return false; }

 protected:
  std::string arrayLabel;
  Addr simVaddr;
  size_t size;
};

class SetArrayMemoryTypeCmd : public AcceleratorCommand {
 public:
  SetArrayMemoryTypeCmd(const std::string& _arrayLabel, MemoryType _memType)
      : AcceleratorCommand(), arrayLabel(_arrayLabel), memType(_memType) {}

  void run(Gem5Datapath* accel) override {
    accel->setArrayMemoryType(arrayLabel, memType);
  }

  std::string name() const override { return "SetArrayMemoryType command"; }

  bool blocking() const override { return false; }

 protected:
  std::string arrayLabel;
  MemoryType memType;
};

#endif
