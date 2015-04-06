#ifndef ALADDIN_TLB_HH
#define ALADDIN_TLB_HH

#include <map>
#include <set>
#include <deque>
#include <unordered_map>

#include "base/statistics.hh"
#include "mem/mem_object.hh"
#include "mem/request.hh"

class CacheDatapath;

class AladdinTLBEntry
{
  public:
    Addr vpn;
    Addr ppn;
    bool free;
    Tick mruTick;
    uint32_t hits;

    AladdinTLBEntry() : vpn(0), ppn(0), free(true), mruTick(0), hits(0){}
    AladdinTLBEntry(AladdinTLBEntry& entry) :
        vpn(entry.vpn), ppn(entry.ppn), free(entry.free),
        mruTick(entry.mruTick), hits(entry.hits) {}

    void setMRU() {mruTick = curTick();}
};

class BaseTLBMemory {

public:
    virtual ~BaseTLBMemory(){}
    virtual bool lookup(Addr vpn, Addr& ppn, bool set_mru=true) = 0;
    /* Inserts a translation into the TLB and returns a newly allocated pointer
     * to the translation entry that was evicted, or NULL if there was no
     * eviction. The caller is responsible for destroying the pointer.
     */
    virtual AladdinTLBEntry* insert(Addr vpn, Addr ppn) = 0;
    // Name of the TLB structure for printing traces.
};

class TLBMemory : public BaseTLBMemory {
    int numEntries;
    int sets;
    int ways;
    int pageBytes;

    AladdinTLBEntry **entries;

protected:
    TLBMemory() {}

public:
    TLBMemory(int _numEntries, int associativity, int _pageBytes) :
        numEntries(_numEntries), sets(associativity), pageBytes(_pageBytes)
    {
        if (sets == 0) {
            sets = numEntries;
        }
        assert(numEntries % sets == 0);
        ways = numEntries/sets;
        entries = new AladdinTLBEntry*[ways];
        for (int i=0; i < ways; i++) {
            entries[i] = new AladdinTLBEntry[sets];
        }
    }
    ~TLBMemory()
    {
        for (int i=0; i < sets; i++) {
            delete[] entries[i];
        }
        delete[] entries;
    }

    virtual bool lookup(Addr vpn, Addr& ppn, bool set_mru=true);
    virtual AladdinTLBEntry* insert(Addr vpn, Addr ppn);
};

class InfiniteTLBMemory : public BaseTLBMemory {
    std::map<Addr, Addr> entries;
public:
    InfiniteTLBMemory() {}
    ~InfiniteTLBMemory() {}

    bool lookup(Addr vpn, Addr& ppn, bool set_mru=true)
    {
        auto it = entries.find(vpn);
        if (it != entries.end()) {
            ppn = it->second;
            return true;
        } else {
            ppn = Addr(0);
            return false;
        }
    }

    virtual AladdinTLBEntry* insert(Addr vpn, Addr ppn)
    {
        entries[vpn] = ppn;
        return nullptr;  // Infinite TLB never has to evict.
    }
};

class AladdinTLB
{
  private:
    CacheDatapath *datapath;
    void regStats(std::string accelerator_name);

    unsigned numEntries;
    unsigned assoc;
    Cycles hitLatency;
    Cycles missLatency;
    Addr pageBytes;
    bool isPerfectTLB;
    unsigned numOutStandingWalks;
    unsigned numOccupiedMissQueueEntries;
    std::string cacti_cfg;  // CACTI 6.5+ config file.

    // Power from CACTI.
    float readEnergy;
    float writeEnergy;
    float leakagePower;
    float area;

    BaseTLBMemory *tlbMemory;

    class deHitQueueEvent : public Event
    {
      public:
        /*Constructs a deHitQueueEvent*/
        deHitQueueEvent(AladdinTLB *_tlb);
        /*Processes the event*/
        void process();
        /*Returns the description of this event*/
        const char *description() const;
        /* Returns name of this event. */
        virtual const std::string name() const;
      private:
        /* The pointer the to AladdinTLB unit*/
        AladdinTLB *tlb;
    };

    class outStandingWalkReturnEvent : public Event
    {
      public:
        /*Constructs a outStandingWalkReturnEvent*/
        outStandingWalkReturnEvent(AladdinTLB *_tlb);
        /*Processes the event*/
        void process();
        /*Returns the description of this event*/
        const char *description() const;
        /* Returns name of this event. */
        virtual const std::string name() const;
      private:
        /* The pointer to the AladdinTLB unit*/
        AladdinTLB *tlb;
    };

    std::deque<PacketPtr> hitQueue;
    std::deque<Addr> outStandingWalks;
    std::unordered_multimap<Addr, PacketPtr> missQueue;

    /* Stores all explicitly mapped TLB translations. Later, we should move this
     * functionality into gem5's page table model. For now, we'll keep them
     * separate and just model the timing.
     */
    std::map<Addr, Addr> infiniteBackupTLB;

    /* Translations from trace base addresses to simulated virtual addresses.
     *
     * This is purely an artifact of how Aladdin is implemented so it doesn't
     * need to be modeled for timing.
     */
    std::map<Addr, Addr> traceToVirtualAddrMap;

  public:
    AladdinTLB(CacheDatapath *_datapath, unsigned _num_entries, unsigned _assoc,
               Cycles _hit_latency, Cycles _miss_latency, Addr pageBytes,
               bool _is_perfect, unsigned _num_walks, unsigned _bandwidth,
               std::string _cacti_config, std::string _accelerator_name);
    ~AladdinTLB();

    std::string name() const;

    /* Getters. */
    unsigned getNumEntries() { return numEntries; }
    unsigned getAssoc() { return assoc; }
    Cycles getHitLatency() { return hitLatency; }
    Cycles getMissLatency() { return missLatency; }
    Addr getPageBytes() { return pageBytes; }
    Addr pageMask() { return pageBytes - 1; };
    bool getIsPerfectTLB() { return isPerfectTLB; }
    unsigned getNumOutStandingWalks() { return numOutStandingWalks; }

    /* Inserts a translation between simulated virtual and physical pages.
     *
     * The simulated virtual pages are NOT obtained from the trace - this is the
     * simulated virtual address space of the simulator.
     *
     * Note that the entries are actually page-aligned addresses instead of just
     * page numbers (e.g. 0xf000 instead of 0xf). If the insert results in an
     * eviction, the evicted entry is returned.
     */
    AladdinTLBEntry* insert(Addr vpn, Addr ppn);

    /* Translates trace addresses to simulated virtual addresses.
     *
     * Because trace addresses and the simulated addresses live in entirely
     * disjunct address spaces, we need more than just bit manipulations to
     * properly translate trace address to physical addresses. Use this
     * translation to get the simulated virtual address, and then consult the
     * TLB to get the simulated physical address.
     */
    void insertTraceToVirtual(Addr trace_addr, Addr sim_vaddr)
    {
      traceToVirtualAddrMap[trace_addr] = sim_vaddr;
    }

    Addr lookupVirtualAddr(Addr trace_addr)
    {
      return traceToVirtualAddrMap[trace_addr];
    }

    bool translateTiming(PacketPtr pkt);
    bool canRequestTranslation();
    void incrementRequestCounter() { requests_this_cycle ++; }
    void resetRequestCounter() { requests_this_cycle = 0; }

    /* Power and area calculations. */
    void computeCactiResults();
    void getAveragePower(
        unsigned int cycles, unsigned int cycleTime,
        float *avg_power, float *avg_dynamic, float *avg_leak);
    float getArea() { return area; }

    class TLBSenderState : public Packet::SenderState
    {
      public:
        TLBSenderState(unsigned _node_id) : node_id(_node_id) {}
        unsigned node_id;
    };

    /* Number of TLB translation requests in the current cycle. */
    unsigned requests_this_cycle;
    /* Maximum translation requests per cycle. */
    unsigned bandwidth;

    Stats::Scalar hits;
    Stats::Scalar misses;
    Stats::Scalar reads;
    Stats::Scalar updates;
    Stats::Formula hitRate;

};

#endif
