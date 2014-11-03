#ifndef __SCRATCHPAD_DATAPATH__
#define __SCRATCHPAD_DATAPATH__

#include <string>

//gem5 Headers
#include "base/types.hh"
#include "base/trace.hh"
#include "base/flags.hh"
#include "mem/request.hh"
#include "mem/packet.hh"
#include "mem/mem_object.hh"
#include "sim/system.hh"
#include "sim/sim_exit.hh"
#include "sim/clocked_object.hh"
#include "params/CacheDatapath.hh"

#include "aladdin/common/BaseDatapath.h"
#include "aladdin_tlb.hh"

#define MASK 0x7fffffff
#define MAX_INFLIGHT_NODES 100

/* Multiple inheritance is unfortunately unavoidable here. BaseDatapath is
 * meant to generalize for integration with any simulator, and in this
 * particular case, GEM5 requires us to interface with its APIs. I don't see any
 * way to avoid this without changing Aladdin to be more GEM5 specific, which
 * isn't a great option either.
 *
 * Fortunately for us, BaseDatapath and MemObject are entirely separate and do
 * not have any name collisions, so as long as we keep it that way and we use
 * the right interfaces in the right places, we'll be fine.
 */
class CacheDatapath : public BaseDatapath, public MemObject {

  public:
    typedef CacheDatapathParams Params;

    CacheDatapath(const Params *p);
    virtual ~CacheDatapath();

    virtual MasterPort &getDataPort(){return dcachePort;};
    MasterID dataMasterId() {return _dataMasterId;};

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

    void finishTranslation(PacketPtr pkt);

    void parse_config();
    bool fileExists (const string file_name)
    {
      struct stat buf;
      if (stat(file_name.c_str(), &buf) != -1)
        return true;
      return false;
    }

  private:
    // TODO: The XIOSim integration has something very similar to this. We
    // should be able to combine them into a common data type.
    typedef enum {
        Ready,
        Translating,
        Translated,
        WaitingFromCache,
        Returned
    } MemAccessStatus;
    typedef uint32_t FlagsType;

    /* Stores the number of load, stores, or TLB requests in flight as well as
     * the size of the corresponding queue. */
    struct mem_queue
    {
      const int size;  // Size of the queue.
      const int bandwidth;  // Max requests per cycle.
      int in_flight;  // Number of requests in the queue.
      int issued_this_cycle;  // Number of requests issued in the current cycle.

      mem_queue(int _size, int _bandwidth):
          size(_size), bandwidth(_bandwidth),
          in_flight(0), issued_this_cycle(0) {}

      /* Returns true if we have not exceeded the cache's bandwidth or the size
       * of the request queue.
       */
      bool can_issue()
      {
        return (in_flight < size) && (issued_this_cycle < bandwidth);
      }
    };

    // Wrapper for step() to match the function signature required by
    // EventWrapper.
    void event_step();
    bool step();
    void dumpStats();
    void copyToExecutingQueue();
    void stepExecutingQueue();
    void globalOptimizationPass();
    void initActualAddress();
    void resetCacheCounters();

    class Node
    {
      public:
        Node (unsigned _node_id) : node_id(_node_id){};
        ~Node();
      private:
        unsigned node_id;
    };

    class DcachePort : public MasterPort
    {
      public:
        DcachePort (CacheDatapath * _datapath)
          : MasterPort(_datapath->name() + ".dcache_port", _datapath),
            datapath(_datapath) {}
      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvTimingSnoopReq(PacketPtr pkt);
        virtual void recvFunctionalSnoop(PacketPtr pkt){}
        virtual Tick recvAtomicSnoop(PacketPtr pkt) {return 0;}
        virtual void recvRetry();
        virtual bool isSnooping() const {return true;}
        CacheDatapath *datapath;
    };

    void completeDataAccess(PacketPtr pkt);

    MasterID _dataMasterId;

    //const unsigned int _cacheLineSize;
    DcachePort dcachePort;

    //gem5 tick
    EventWrapper<CacheDatapath, &CacheDatapath::event_step> tickEvent;

    class DatapathSenderState : public Packet::SenderState
    {
      public:
        DatapathSenderState(unsigned _node_id) : node_id(_node_id) {}
        unsigned node_id;
    };
    PacketPtr retryPkt;

    /* True if the cache's MSHRs are full. */
    bool isCacheBlocked;

    /* Actual memory request addresses, obtained from the trace. */
    std::unordered_map<unsigned, pair<Addr, uint8_t> > actualAddress;

    /* TODO(samxi): I'd like to separate the cache interactions into its own
     * class or set of methods, a la what I am doing with XIOSim, but right now
     * that can't be a priority.
     */
    std::map<unsigned, MemAccessStatus> mem_accesses;
    /* Load and store queues. */
    mem_queue load_queue;
    mem_queue store_queue;

    bool accessTLB(Addr addr, unsigned size, bool isLoad, int node_id);
    bool accessCache(Addr addr, unsigned size, bool isLoad, int node_id);
    void writeTLBStats();

    AladdinTLB dtb;

    unsigned inFlightNodes;

    System *system;

};

#endif
