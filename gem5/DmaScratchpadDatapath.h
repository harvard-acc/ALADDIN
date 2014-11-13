#ifndef __DMA_SCRATCHPAD_DATAPATH_H__
#define __DMA_SCRATCHPAD_DATAPATH_H__

#include <string>
#include <deque>
#include "base/types.hh"
#include "base/trace.hh"
#include "base/flags.hh"
#include "dev/dma_device.hh"
#include "mem/request.hh"
#include "mem/packet.hh"
#include "mem/mem_object.hh"
#include "sim/system.hh"
#include "sim/sim_exit.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"

#include "aladdin/common/ScratchpadDatapath.h"
#include "params/DmaScratchpadDatapath.hh"

// TODO: Refactor.
#define MASK 0x7fffffff
#define MAX_INFLIGHT_NODES 100

class DmaScratchpadDatapath : public ScratchpadDatapath, public MemObject {

  public:
    DmaScratchpadDatapath(const DmaScratchpadDatapathParams* params);
    ~DmaScratchpadDatapath();

    virtual MasterPort &getDataPort(){ return spadPort; };
    MasterID dataMasterId() { return _dataMasterId; };

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
    BaseMasterPort &getMasterPort(const std::string &if_name,
                                  PortID idx = InvalidPortID);

    /* Wrapper function for the real step function to match EventWrapper
     * interface.
     */
    void event_step();
    bool step();
    void stepExecutingQueue();
    void initActualAddress();

    /* DMA access functions. */
    void issueDmaRequest(Addr addr, unsigned size, bool isLoad, int node_id);
    void completeDmaAccess(Addr addr);

  private:
    typedef enum {
        Ready,
        Waiting,
        Returned
    } DmaRequestStatus;
    std::map<unsigned, DmaRequestStatus> dma_requests;
    std::unordered_map<unsigned, pair<Addr, unsigned> > actualAddress;
    int inFlightNodes;

    /* This port has to accept snoops but it doesn't need to do anything. See
     * arch/arm/table_walker.hh
     */
    class SpadPort : public DmaPort
    {
      public:
        SpadPort (DmaScratchpadDatapath *dev, System *s) :
            DmaPort(dev, s), _datapath(dev) {}
      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvTimingSnoopReq(PacketPtr pkt) {}
        virtual void recvFunctionalSnoop(PacketPtr pkt){}
        virtual bool isSnooping() const { return true; }
        DmaScratchpadDatapath* _datapath;
    };
    class DmaEvent : public Event
    {
      private:
        DmaScratchpadDatapath *datapath;
      public:
        /** Constructor */
        DmaEvent(DmaScratchpadDatapath *_dpath);
        /** Process a dma event */
        void process();
        /** Returns the description of the tick event. */
        const char *description() const;
    };
    /** To store outstanding DMA requests. */
    std::deque<Addr> dmaQueue;

    MasterID _dataMasterId;

    //const unsigned int _cacheLineSize;
    SpadPort spadPort;

    //gem5 tick
    EventWrapper<DmaScratchpadDatapath,
                 &DmaScratchpadDatapath::event_step> tickEvent;
    // Number of cycles required for the CPU to reinitiate a DMA transfer.
    unsigned dmaSetupLatency;
    System *system;
};

#endif
