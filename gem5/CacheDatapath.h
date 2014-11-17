#ifndef __SCRATCHPAD_DATAPATH__
#define __SCRATCHPAD_DATAPATH__

#include <string>

//gem5 Headers
#include "base/flags.hh"
#include "base/statistics.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "mem/mem_object.hh"
#include "mem/packet.hh"
#include "mem/request.hh"
#include "sim/clocked_object.hh"
#include "sim/sim_exit.hh"
#include "sim/system.hh"

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
 *
 * TODO: Collapse BaseDatapath and MemoryInterface into a single class. Then we
 * can get rid of MemoryInterface entirely.
 */
class CacheDatapath :
    public BaseDatapath, public MemObject, public MemoryInterface {

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

    /* Complete the translation process. @was_miss is true if the request
     * originally missed, because the datapath must retry the instruction
     * instead of directly receiving the translation.
     */
    void finishTranslation(PacketPtr pkt, bool was_miss);

    void parse_config();
    bool fileExists (const string file_name)
    {
      struct stat buf;
      if (stat(file_name.c_str(), &buf) != -1)
        return true;
      return false;
    }

    /* Generic memory interface functions. */
    void computeCactiResults();
    void getAveragePower(unsigned int cycles, float *avg_power,
                         float *avg_dynamic, float *avg_leak);
    void getMemoryBlocks(std::vector<std::string>& names);
    void getRegisterBlocks(std::vector<std::string>& names);
    float getTotalArea();
    float getReadEnergy(std::string block_name);
    float getWriteEnergy(std::string block_name);
    float getLeakagePower(std::string block_name);
    float getArea(std::string block_name);

    /* Stores the number of load, stores, or TLB requests in flight as well as
     * the size of the corresponding queue. */
    class MemoryQueue
    {
      public:
        MemoryQueue(int _size,
                    int _bandwidth,
                    std::string _name,
                    std::string _cacti_config):
            size(_size), bandwidth(_bandwidth),
            in_flight(0), issued_this_cycle(0),
            name(_name), cacti_config(_cacti_config)
        {
          readStats.name(name + "_reads")
                   .desc("Number of reads to the " + name)
                   .flags(Stats::total | Stats::nonan);
          writeStats.name(name + "_writes")
                    .desc("Number of writes to the " + name)
                    .flags(Stats::total | Stats::nonan);
        }

        /* Returns true if we have not exceeded the cache's bandwidth or the
         * size of the request queue.
         */
        bool can_issue()
        {
          return (in_flight < size) && (issued_this_cycle < bandwidth);
        }

        void computeCactiResults()
        {
          uca_org_t cacti_result = cacti_interface(cacti_config);
          readEnergy = cacti_result.power.readOp.dynamic * 1e9;
          writeEnergy = cacti_result.power.writeOp.dynamic * 1e9;
          leakagePower = cacti_result.power.readOp.leakage * 1000;
          area = cacti_result.area;
        }

        void getAveragePower(
            unsigned int cycles, unsigned int cycleTime,
            float *avg_power, float *avg_dynamic, float *avg_leak)
        {
          *avg_dynamic = (readStats.value() * readEnergy +
                         writeStats.value() * writeEnergy) /
                        (cycles * cycleTime);
          *avg_leak = leakagePower;
          *avg_power = *avg_dynamic + *avg_leak;
        }

        const int size;  // Size of the queue.
        const int bandwidth;  // Max requests per cycle.
        int in_flight;  // Number of requests in the queue.
        int issued_this_cycle;  // Requests issued in the current cycle.
        Stats::Scalar readStats;
        Stats::Scalar writeStats;

      private:
        std::string name;  // Specifies whether this is a load or store queue.
        std::string cacti_config;  // CACTI config file.
        float readEnergy;
        float writeEnergy;
        float leakagePower;
        float area;
    };

    /* Load and store queues. */
    MemoryQueue load_queue;
    MemoryQueue store_queue;
    // TODO: These are provided by GEM5 directly but I have no idea how to
    // access them. Based on an extensive Google search, there doesn't seem like
    // a good answer even exists - all the power modeling appears to be done
    // after the fact rather than during runtime. For now, I'm just going to
    // track total loads and stores, regardless of whether they hit or miss.
    // Simulations confirm that they agree with GEM5's cache stats.
    Stats::Scalar loads;
    Stats::Scalar stores;

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

    // Register accelerator statistics.
    void registerStats();
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

    /* CACTI configuration file for the main cache. */
    std::string cacti_cfg;

    bool accessTLB(Addr addr, unsigned size, bool isLoad, int node_id);
    bool accessCache(Addr addr, unsigned size, bool isLoad, int node_id);

    AladdinTLB dtb;

    /* Energy and area numbers generated by CACTI. */
    float readEnergy;
    float writeEnergy;
    float leakagePower;
    float area;

    unsigned inFlightNodes;

    System *system;

};

#endif
