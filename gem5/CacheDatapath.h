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

#include "aladdin/common/DatabaseDeps.h"
#include "aladdin/common/BaseDatapath.h"
#include "aladdin_tlb.hh"
#include "Gem5Datapath.h"

#include "params/CacheDatapath.hh"

#define MAX_INFLIGHT_NODES 100
/*hack, to fix*/
#define MIN_CACTI_SIZE 64

/* Multiple inheritance is unfortunately unavoidable here. BaseDatapath is
 * meant to generalize for integration with any simulator, and in this
 * particular case, GEM5 requires us to interface with its APIs. I don't see any
 * way to avoid this without changing Aladdin to be more GEM5 specific, which
 * isn't a great option either. However, the two base classes are entirely
 * separate in terms of their ancestry, which makes this less evil.
 */
class CacheDatapath :
    public BaseDatapath, public Gem5Datapath {

  public:
    typedef CacheDatapathParams Params;

    CacheDatapath(const Params *p);
    virtual ~CacheDatapath();

    virtual MasterPort& getDataPort() { return dcachePort; };
    virtual MasterID dataMasterId() { return _dataMasterId; };

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
    virtual Addr getBaseAddress(std::string label);

    void parse_config();
    bool fileExists (const string file_name)
    {
      struct stat buf;
      if (stat(file_name.c_str(), &buf) != -1)
        return true;
      return false;
    }

    /* Memory interface functions. */
    void getAverageMemPower(unsigned int cycles, float *avg_power,
                            float *avg_dynamic, float *avg_leak);
    double getTotalMemArea();
    void computeCactiResults();
    void getMemoryBlocks(std::vector<std::string>& names);
    void getRegisterBlocks(std::vector<std::string>& names);
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
          if (size >= MIN_CACTI_SIZE)
          {
            readEnergy = cacti_result.power.readOp.dynamic * 1e9;
            writeEnergy = cacti_result.power.writeOp.dynamic * 1e9;
            leakagePower = cacti_result.power.readOp.leakage * 1000;
            area = cacti_result.area;
          }
          else
          {
            /*Assuming it scales linearly with cache size*/
            readEnergy = cacti_result.power.readOp.dynamic * 1e9 * size / MIN_CACTI_SIZE;
            writeEnergy = cacti_result.power.writeOp.dynamic * 1e9 * size / MIN_CACTI_SIZE;
            leakagePower = cacti_result.power.readOp.leakage * 1000 * size / MIN_CACTI_SIZE;
            area = cacti_result.area * size / MIN_CACTI_SIZE;
          }
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
        float getArea() {return area;}

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

  protected:
#ifdef USE_DB
    int writeConfiguration(sql::Connection *con);
#endif

  private:
    typedef enum {
        Ready,
        Translating,
        Translated,
        WaitingFromCache,
        Returned
    } MemAccessStatus;

    struct InFlightMemAccess {
      // Current status of the memory request.
      MemAccessStatus status;
      // Translated physical address.
      Addr paddr;
    };

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
    Event& getTickEvent() { return tickEvent; }

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

    /* Insert a new virtual to physical address mapping into the Aladdin TLB.
     */
    virtual void insertTLBEntry(Addr trace_addr, Addr vaddr, Addr paddr);

    MasterID _dataMasterId;

    DcachePort dcachePort;

    //gem5 tick
    EventWrapper<CacheDatapath, &CacheDatapath::event_step> tickEvent;

    class DatapathSenderState : public Packet::SenderState
    {
      public:
        DatapathSenderState(unsigned _node_id, bool _is_ctrl_signal = false) :
            node_id(_node_id),
            is_ctrl_signal(_is_ctrl_signal) {}

        /* Aladdin node that triggered the memory access. */
        unsigned node_id;

        /* Flag that determines whether a packet received on a data port is a
         * control signal accessed through memory (which needs to be handled
         * differently) or an ordinary memory access.
         */
        bool is_ctrl_signal;
    };

    PacketPtr retryPkt;

    /* True if the cache's MSHRs are full. */
    bool isCacheBlocked;

    /* L1 cache parameters. */
    std::string cacheSize;  // Because of GEM5's stupid xxkB format.
    unsigned cacheLineSize;
    unsigned cacheHitLatency;
    unsigned cacheAssoc;

    /* Stores status information about memory accesses currently in flight. */
    std::map<unsigned, InFlightMemAccess> inflight_mem_ops;

    /* CACTI configuration file for the main cache. */
    std::string cacti_cfg;

    // Accelerator ID assigned by the system.
    // Unique datapath name based on the accelerator_id.
    std::string accelerator_name;

    bool accessTLB(Addr addr, unsigned size, bool isLoad, int node_id);
    bool accessCache(Addr addr, unsigned size, bool isLoad, int node_id,
                     long long int value);

    virtual void sendFinishedSignal();

    AladdinTLB dtb;

    /* Energy and area numbers generated by CACTI. */
    float readEnergy;
    float writeEnergy;
    float leakagePower;
    float area;

    unsigned inFlightNodes;
};

#endif
