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
    void setMRU() {mruTick = curTick();}
};

class BaseTLBMemory {

public:
    virtual ~BaseTLBMemory(){}
    virtual bool lookup(Addr vpn, Addr& ppn, bool set_mru=true) = 0;
    virtual void insert(Addr vpn, Addr ppn) = 0;
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
    virtual void insert(Addr vpn, Addr ppn);
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
    void insert(Addr vpn, Addr ppn)
    {
        entries[vpn] = ppn;
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
      private:
        /* The pointer the to AladdinTLB unit*/
        AladdinTLB *tlb;
    };

    std::deque<PacketPtr> hitQueue;
    std::deque<Addr> outStandingWalks;
    std::unordered_multimap<Addr, PacketPtr> missQueue;

  public:
    AladdinTLB(CacheDatapath *_datapath, unsigned _num_entries, unsigned _assoc,
               Cycles _hit_latency, Cycles _miss_latency, Addr pageBytes,
               bool _is_perfect, unsigned _num_walks, unsigned _bandwidth,
               std::string _cacti_config, std::string _accelerator_name);
    ~AladdinTLB();

    std::string name() const;

    bool translateTiming(PacketPtr pkt);

    void insert(Addr vpn, Addr ppn);

    bool canRequestTranslation();
    void incrementRequestCounter() { requests_this_cycle ++; }
    void resetRequestCounter() { requests_this_cycle = 0; }
    void computeCactiResults();
    void getAveragePower(
        unsigned int cycles, unsigned int cycleTime,
        float *avg_power, float *avg_dynamic, float *avg_leak);
    float getArea() {return area;}
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
