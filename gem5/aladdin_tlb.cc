#include <string>

#include "aladdin/common/cacti-p/cacti_interface.h"
#include "aladdin/common/cacti-p/io.h"

#include "aladdin_tlb.hh"
#include "CacheDatapath.h"
#include "debug/CacheDatapath.hh"
/*hack, to fix*/
#define MIN_CACTI_SIZE 64

AladdinTLB::AladdinTLB(
    CacheDatapath *_datapath, unsigned _num_entries, unsigned _assoc,
    Cycles _hit_latency, Cycles _miss_latency, Addr _page_bytes,
    bool _is_perfect, unsigned _num_walks, unsigned _bandwidth,
    std::string _cacti_config, std::string _accelerator_name) :
  datapath(_datapath),
  numEntries(_num_entries),
  assoc(_assoc),
  hitLatency(_hit_latency),
  missLatency(_miss_latency),
  pageBytes(_page_bytes),
  isPerfectTLB(_is_perfect),
  numOutStandingWalks(_num_walks),
  numOccupiedMissQueueEntries(0),
  cacti_cfg(_cacti_config),
  requests_this_cycle(0),
  bandwidth(_bandwidth)
{
  if (numEntries > 0)
    tlbMemory = new TLBMemory (_num_entries, _assoc, _page_bytes);
  else
    tlbMemory = new InfiniteTLBMemory();
  regStats(_accelerator_name);
}

AladdinTLB::~AladdinTLB()
{
  delete tlbMemory;
}

AladdinTLB::deHitQueueEvent::deHitQueueEvent(AladdinTLB *_tlb)
   : Event(Default_Pri, AutoDelete),
     tlb(_tlb) {}

void
AladdinTLB::deHitQueueEvent::process()
{
  assert(!tlb->hitQueue.empty());
  tlb->datapath->finishTranslation(tlb->hitQueue.front(), false);
  tlb->hitQueue.pop_front();
}

const char *
AladdinTLB::deHitQueueEvent::description() const
{
  return "TLB Hit";
}


const std::string
AladdinTLB::deHitQueueEvent::name() const
{
  return tlb->name() + ".dehitqueue_event";
}

AladdinTLB::outStandingWalkReturnEvent::outStandingWalkReturnEvent(
    AladdinTLB *_tlb) : Event(Default_Pri, AutoDelete), tlb(_tlb) {}

void
AladdinTLB::outStandingWalkReturnEvent::process()
{
  // TLB return events are free because only the CPU's hardware control units
  // can write to the TLB; programs can only read the TLB.
  assert(!tlb->missQueue.empty());
  Addr vpn = tlb->outStandingWalks.front();
  // Retrieve translation from the infinite backup storage if it exists;
  // otherwise, just use ppn=vpn.
  Addr ppn;
  if (tlb->infiniteBackupTLB.find(vpn) != tlb->infiniteBackupTLB.end()) {
    DPRINTF(CacheDatapath, "TLB miss was resolved in the backup TLB.\n");
    ppn = tlb->infiniteBackupTLB[vpn];
  } else {
    DPRINTF(CacheDatapath, "TLB miss was not resolved in the backup TLB.\n");
    ppn = vpn;
  }
  DPRINTF(CacheDatapath, "Translated vpn %#x -> ppn %#x.\n", vpn, ppn);
  AladdinTLBEntry* evicted = tlb->insert(vpn, ppn);
  if (evicted) {
    DPRINTF(CacheDatapath, "TLB fill after page table walk evicted the entry "
            "vpn %#x -> ppn %#x.\n", evicted->vpn, evicted->ppn);
    tlb->infiniteBackupTLB[evicted->vpn] = evicted->ppn;
  }

  auto range = tlb->missQueue.equal_range(vpn);
  for(auto it = range.first; it!= range.second; ++it)
    tlb->datapath->finishTranslation(it->second, true);

  tlb->numOccupiedMissQueueEntries --;
  tlb->missQueue.erase(vpn);
  tlb->outStandingWalks.pop_front();
  tlb->updates++;  // Upon completion, increment TLB
}

const char *
AladdinTLB::outStandingWalkReturnEvent::description() const
{
  return "TLB Miss";
}

const std::string
AladdinTLB::outStandingWalkReturnEvent::name() const
{
  return tlb->name() + ".page_walk_event";
}

bool
AladdinTLB::translateTiming(PacketPtr pkt)
{
  Addr vaddr = pkt->req->getPaddr();
  DPRINTF(CacheDatapath, "Translating vaddr %#x.\n", vaddr);
  Addr offset = vaddr % pageBytes;
  Addr vpn = vaddr - offset;
  Addr ppn;

  reads++;  // Both TLB hits and misses perform a read.
  if (isPerfectTLB || tlbMemory->lookup(vpn, ppn))
  {
      DPRINTF(CacheDatapath, "TLB hit. Phys addr %#x.\n", ppn + offset);
      hits++;
      hitQueue.push_back(pkt);
      deHitQueueEvent *hq = new deHitQueueEvent(this);
      datapath->schedule(hq, datapath->clockEdge(hitLatency));
      *(pkt->getPtr<Addr>()) = ppn;
      return true;
  }
  else
  {
      // TLB miss! Let the TLB handle the walk, etc
      DPRINTF(CacheDatapath, "TLB miss for addr %#x\n", vaddr);

      if (missQueue.find(vpn) == missQueue.end())
      {
        if (numOutStandingWalks != 0 &&
            outStandingWalks.size() >= numOutStandingWalks)
          return false;
        outStandingWalks.push_back(vpn);
        outStandingWalkReturnEvent *mq = new outStandingWalkReturnEvent(this);
        datapath->schedule(mq, datapath->clockEdge(missLatency));
        numOccupiedMissQueueEntries ++;
        DPRINTF(CacheDatapath, "Allocated TLB miss entry for addr %#x, page %#x\n",
                vaddr, vpn);
      }
      else
      {
        DPRINTF(CacheDatapath, "Collapsed into existing miss entry for page %#x\n", vpn);
      }
      misses++;
      missQueue.insert({vpn, pkt});
      return true;
  }
}

AladdinTLBEntry*
AladdinTLB::insert(Addr vpn, Addr ppn)
{
    AladdinTLBEntry* evicted = tlbMemory->insert(vpn, ppn);
    infiniteBackupTLB[vpn] = ppn;
    return evicted;
}

bool
AladdinTLB::canRequestTranslation()
{
  return requests_this_cycle < bandwidth &&
         numOccupiedMissQueueEntries < numOutStandingWalks;
}

void
AladdinTLB::regStats(std::string accelerator_name)
{
  using namespace Stats;
  hits.name("system." + accelerator_name + ".tlb.hits")
      .desc("TLB hits")
      .flags(total | nonan);
  misses.name("system." + accelerator_name + ".tlb.misses")
        .desc("TLB misses")
        .flags(total | nonan);
  hitRate.name("system." + accelerator_name + ".tlb.hitRate")
         .desc("TLB hit rate")
         .flags(total | nonan);
  hitRate = hits / (hits + misses);
  reads.name("system." + accelerator_name + ".tlb.reads")
       .desc("TLB reads")
       .flags(total | nonan);
  updates.name("system." + accelerator_name + ".tlb.updates")
         .desc("TLB updates")
         .flags(total | nonan);
}

void
AladdinTLB::computeCactiResults()
{
  DPRINTF(CacheDatapath, "Invoking CACTI for TLB power and area estimates.\n");
  uca_org_t cacti_result = cacti_interface(cacti_cfg);
  if (numEntries >= MIN_CACTI_SIZE)
  {
    readEnergy = cacti_result.power.readOp.dynamic * 1e9;
    writeEnergy = cacti_result.power.writeOp.dynamic * 1e9;
    leakagePower = cacti_result.power.readOp.leakage * 1000;
    area = cacti_result.area;
  }
  else
  {
    /*Assuming it scales linearly with cache size*/
    readEnergy = cacti_result.power.readOp.dynamic * 1e9 * numEntries / MIN_CACTI_SIZE;
    writeEnergy = cacti_result.power.writeOp.dynamic * 1e9 * numEntries / MIN_CACTI_SIZE;
    leakagePower = cacti_result.power.readOp.leakage * 1000 * numEntries / MIN_CACTI_SIZE;
    area = cacti_result.area * numEntries / MIN_CACTI_SIZE;
  }
}

void
AladdinTLB::getAveragePower(
    unsigned int cycles, unsigned int cycleTime,
    float* avg_power, float* avg_dynamic, float* avg_leak)
{
  *avg_dynamic = (reads.value() * readEnergy + updates.value() * writeEnergy) /
                 (cycles * cycleTime);
  *avg_leak = leakagePower;
  *avg_power = *avg_dynamic + *avg_leak;
}

std::string
AladdinTLB::name() const
{
  return datapath->name() + ".tlb";
}

bool
TLBMemory::lookup(Addr vpn, Addr& ppn, bool set_mru)
{
    int way = (vpn / pageBytes) % ways;
    for (int i=0; i < sets; i++) {
        if (entries[way][i].vpn == vpn && !entries[way][i].free) {
            ppn = entries[way][i].ppn;
            assert(entries[way][i].mruTick > 0);
            if (set_mru) {
                entries[way][i].setMRU();
            }
            entries[way][i].hits++;
            return true;
        }
    }
    return false;
}

AladdinTLBEntry*
TLBMemory::insert(Addr vpn, Addr ppn)
{
    Addr a;
    if (lookup(vpn, a)) {
        return nullptr;
    }
    int way = (vpn / pageBytes) % ways;
    AladdinTLBEntry* entry = nullptr;
    AladdinTLBEntry* evicted_entry = nullptr;
    Tick minTick = curTick();
    for (int i=0; i < sets; i++) {
        if (entries[way][i].free) {
            entry = &entries[way][i];
            break;
        } else if (entries[way][i].mruTick <= minTick) {
            minTick = entries[way][i].mruTick;
            entry = &entries[way][i];
        }
    }
    assert(entry);
    if (!entry->free) {
        DPRINTF(CacheDatapath, "Evicting entry for vpn %#x\n", entry->vpn);
        evicted_entry = new AladdinTLBEntry(*entry);
    }

    entry->vpn = vpn;
    entry->ppn = ppn;
    entry->free = false;
    entry->setMRU();

    return evicted_entry;
}
