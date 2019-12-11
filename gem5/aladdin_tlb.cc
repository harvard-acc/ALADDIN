#include <string>
#include <utility>

#include "aladdin/common/cacti-p/cacti_interface.h"
#include "aladdin/common/cacti-p/io.h"

#include "aladdin_tlb.hh"
#include "HybridDatapath.h"

#include "debug/AladdinTLB.hh"

/*hack, to fix*/
#define MIN_CACTI_SIZE 64

AladdinTLB::AladdinTLB(HybridDatapath* _datapath,
                       unsigned _num_entries,
                       unsigned _assoc,
                       Cycles _hit_latency,
                       Cycles _miss_latency,
                       Addr _page_bytes,
                       unsigned _num_walks,
                       unsigned _bandwidth,
                       std::string _cacti_config,
                       std::string _accelerator_name)
    : datapath(_datapath), numEntries(_num_entries), assoc(_assoc),
      hitLatency(_hit_latency), missLatency(_miss_latency),
      pageBytes(_page_bytes),
      numOutStandingWalks(_num_walks), numOccupiedMissQueueEntries(0),
      cacti_cfg(_cacti_config), requests_this_cycle(0), bandwidth(_bandwidth) {
  if (numEntries > 0)
    tlbMemory = new TLBMemory(_num_entries, _assoc, _page_bytes);
  else
    tlbMemory = new InfiniteTLBMemory();
  regStats(_accelerator_name);
}

AladdinTLB::~AladdinTLB() { delete tlbMemory; }

void AladdinTLB::clear() {
  infiniteBackupTLB.clear();
  arrayLabelToVirtualAddrMap.clear();
  tlbMemory->clear();
}

void AladdinTLB::resetCounters() {
  reads = 0;
  updates = 0;
}

AladdinTLB::deHitQueueEvent::deHitQueueEvent(AladdinTLB* _tlb)
    : Event(Default_Pri, AutoDelete), tlb(_tlb) {}

void AladdinTLB::deHitQueueEvent::process() {
  assert(!tlb->hitQueue.empty());
  tlb->datapath->completeTLBRequest(tlb->hitQueue.front(), false);
  tlb->hitQueue.pop_front();
}

const char* AladdinTLB::deHitQueueEvent::description() const {
  return "TLB Hit";
}

const std::string AladdinTLB::deHitQueueEvent::name() const {
  return tlb->name() + ".dehitqueue_event";
}

AladdinTLB::outStandingWalkReturnEvent::outStandingWalkReturnEvent(
    AladdinTLB* _tlb)
    : Event(Default_Pri, AutoDelete), tlb(_tlb) {}

void AladdinTLB::outStandingWalkReturnEvent::process() {
  // TLB return events are free because only the CPU's hardware control units
  // can write to the TLB; programs can only read the TLB.
  assert(!tlb->missQueue.empty());
  Addr vpn = tlb->outStandingWalks.front();
  // Retrieve translation from the infinite backup storage if it exists;
  // otherwise, just use ppn=vpn.
  Addr ppn;
  if (tlb->infiniteBackupTLB.find(vpn) != tlb->infiniteBackupTLB.end()) {
    DPRINTF(AladdinTLB, "TLB miss was resolved in the backup TLB.\n");
    ppn = tlb->infiniteBackupTLB[vpn];
  } else {
    DPRINTF(AladdinTLB, "TLB miss was not resolved in the backup TLB.\n");
    ppn = vpn;
  }
  DPRINTF(AladdinTLB, "Translated vpn %#x -> ppn %#x.\n", vpn, ppn);
  tlb->insert(vpn, ppn);

  auto range = tlb->missQueue.equal_range(vpn);
  for (auto it = range.first; it != range.second; ++it)
    tlb->datapath->completeTLBRequest(it->second, true);

  tlb->numOccupiedMissQueueEntries--;
  tlb->missQueue.erase(vpn);
  tlb->outStandingWalks.pop_front();
  tlb->updates++;  // Upon completion, increment TLB
}

const char* AladdinTLB::outStandingWalkReturnEvent::description() const {
  return "TLB Miss";
}

const std::string AladdinTLB::outStandingWalkReturnEvent::name() const {
  return tlb->name() + ".page_walk_event";
}

std::pair<Addr, Addr> AladdinTLB::translateTraceToSimVirtual(PacketPtr pkt) {
  /* A somewhat complex translation process.
   *
   * We use the node id to get the array name of the array being accessed. We
   * then look up the translation from this array name to the simulated
   * virtual address (which represents the head of the array). We also get the
   * base trace address from the same array name. Next, we compute
   * the offset between the accessed trace address and the base trace address
   * and add this offset to the base simulated virtual address.  Finally, we
   * consult the TLB to translate the simulated virtual address to the
   * simulated physical address.
   */
  Addr vaddr, vpn, page_offset;
  TLBSenderState* state = dynamic_cast<TLBSenderState*>(pkt->senderState);
  Addr trace_req_vaddr = pkt->req->getPaddr();
  const std::string& array_name = state->var->get_name();
  Addr base_sim_vaddr = lookupVirtualAddr(array_name);
  Addr base_trace_addr =
      static_cast<Addr>(datapath->getBaseAddress(array_name));
  Addr array_offset = trace_req_vaddr - base_trace_addr;
  vaddr = base_sim_vaddr + array_offset;
  DPRINTF(AladdinTLB,
          "Accessing array %s with base address %#x.\n",
          array_name.c_str(),
          base_sim_vaddr);
  DPRINTF(AladdinTLB, "Translating vaddr %#x.\n", vaddr);
  page_offset = vaddr % pageBytes;
  vpn = vaddr - page_offset;
  return std::make_pair(vpn, page_offset);
}

bool AladdinTLB::translateInvisibly(PacketPtr pkt) {
  Addr vpn, ppn, page_offset;
  auto result = translateTraceToSimVirtual(pkt);
  vpn = result.first;
  page_offset = result.second;
  unsigned size = pkt->req->getSize();
  // Ensure that the entire range is mapped.
  for (Addr curr_vpn = vpn; curr_vpn < vpn + size; curr_vpn += pageBytes) {
    Addr curr_ppn;
    if (!tlbMemory->lookup(curr_vpn, curr_ppn)) {
      if (infiniteBackupTLB.find(vpn) != infiniteBackupTLB.end())
        curr_ppn = infiniteBackupTLB[curr_vpn];
      else
        throw AddressTranslationException(curr_vpn, size);
    }
    // Ensure we return the translation for the starting page.
    if (curr_vpn == vpn) {
      ppn = curr_ppn;
    }
  }

  AladdinTLBResponse* translation = pkt->getPtr<AladdinTLBResponse>();
  translation->vaddr = vpn + page_offset;
  translation->paddr = ppn + page_offset;
  return true;
}

Addr AladdinTLB::translateInvisibly(Addr simVaddr, int size) {
  Addr pageOffset = simVaddr % pageBytes;
  Addr vpn = simVaddr - pageOffset;
  Addr ppn;
  if (!tlbMemory->lookup(vpn, ppn)) {
    if (infiniteBackupTLB.find(vpn) != infiniteBackupTLB.end())
      ppn = infiniteBackupTLB[vpn];
    else
      throw AddressTranslationException(vpn, size);
  }

  return ppn + pageOffset;
}

bool AladdinTLB::translateTiming(PacketPtr pkt) {
  Addr vaddr, vpn, ppn, page_offset;
  auto result = translateTraceToSimVirtual(pkt);
  vpn = result.first;
  page_offset = result.second;
  vaddr = vpn + page_offset;

  reads++;  // Both TLB hits and misses perform a read.
  if (tlbMemory->lookup(vpn, ppn)) {
    DPRINTF(AladdinTLB, "TLB hit. Phys addr %#x.\n", ppn + page_offset);
    hits++;
    hitQueue.push_back(pkt);
    deHitQueueEvent* hq = new deHitQueueEvent(this);
    datapath->schedule(hq, datapath->clockEdge(hitLatency));
    // Due to the complexity of translating trace to virtual address, return
    // the complete address, not just the page number.
    AladdinTLBResponse* translation = pkt->getPtr<AladdinTLBResponse>();
    translation->paddr = ppn + page_offset;
    translation->vaddr = vpn + page_offset;
    return true;
  } else {
    // TLB miss! Let the TLB handle the walk, etc
    DPRINTF(AladdinTLB, "TLB miss for addr %#x\n", vaddr);

    if (missQueue.find(vpn) == missQueue.end()) {
      if (numOutStandingWalks != 0 &&
          outStandingWalks.size() >= numOutStandingWalks)
        return false;
      outStandingWalks.push_back(vpn);
      outStandingWalkReturnEvent* mq = new outStandingWalkReturnEvent(this);
      datapath->schedule(mq, datapath->clockEdge(missLatency));
      numOccupiedMissQueueEntries++;
      DPRINTF(AladdinTLB,
              "Allocated TLB miss entry for addr %#x, page %#x\n",
              vaddr,
              vpn);
    } else {
      DPRINTF(AladdinTLB,
              "Collapsed into existing miss entry for page %#x\n",
              vpn);
    }
    misses++;
    missQueue.insert({ vpn, pkt });
    return true;
  }
}

void AladdinTLB::insert(Addr vpn, Addr ppn) {
  tlbMemory->insert(vpn, ppn);
  infiniteBackupTLB[vpn] = ppn;
}

void AladdinTLB::insertBackupTLB(Addr vpn, Addr ppn) {
  infiniteBackupTLB[vpn] = ppn;
}
/*Always return true if TLB has unlimited bandwidth (bandwidth == 0). Otherwise,
 * we check how many requests have been served so far and how many outstanding
 * page table lookups. */
bool AladdinTLB::canRequestTranslation() {
  if (bandwidth == 0)
    return true;
  return (requests_this_cycle < bandwidth &&
          numOccupiedMissQueueEntries < numOutStandingWalks);
}

void AladdinTLB::regStats(std::string accelerator_name) {
  using namespace Stats;
  hits.name("system." + accelerator_name + ".tlb.hits").desc("TLB hits").flags(
      total | nonan);
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

void AladdinTLB::computeCactiResults() {
  DPRINTF(AladdinTLB, "Invoking CACTI for TLB power and area estimates.\n");
  uca_org_t cacti_result = cacti_interface(cacti_cfg);
  if (numEntries >= MIN_CACTI_SIZE) {
    readEnergy = cacti_result.power.readOp.dynamic * 1e12;
    writeEnergy = cacti_result.power.writeOp.dynamic * 1e12;
    leakagePower = cacti_result.power.readOp.leakage * 1000;
    area = cacti_result.area;
  } else {
    /*Assuming it scales linearly with cache size*/
    readEnergy =
        cacti_result.power.readOp.dynamic * 1e12 * numEntries / MIN_CACTI_SIZE;
    writeEnergy =
        cacti_result.power.writeOp.dynamic * 1e12 * numEntries / MIN_CACTI_SIZE;
    leakagePower =
        cacti_result.power.readOp.leakage * 1000 * numEntries / MIN_CACTI_SIZE;
    area = cacti_result.area * numEntries / MIN_CACTI_SIZE;
  }
}

void AladdinTLB::getAveragePower(unsigned int cycles,
                                 unsigned int cycleTime,
                                 float* avg_power,
                                 float* avg_dynamic,
                                 float* avg_leak) {
  *avg_dynamic = (reads.value() * readEnergy + updates.value() * writeEnergy) /
                 (cycles * cycleTime);
  *avg_leak = leakagePower;
  *avg_power = *avg_dynamic + *avg_leak;
}

std::string AladdinTLB::name() const { return datapath->name() + ".tlb"; }

bool TLBMemory::lookup(Addr vpn, Addr& ppn, bool set_mru) {
  int way = (vpn / pageBytes) % ways;
  for (int i = 0; i < sets; i++) {
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

void TLBMemory::insert(Addr vpn, Addr ppn) {

  Addr a;
  if (lookup(vpn, a)) {
    return;
  }
  int way = (vpn / pageBytes) % ways;
  AladdinTLBEntry* entry = nullptr;
  Tick minTick = curTick();
  for (int i = 0; i < sets; i++) {
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
    DPRINTF(AladdinTLB, "Evicting entry for vpn %#x\n", entry->vpn);
  }

  entry->vpn = vpn;
  entry->ppn = ppn;
  entry->free = false;
  entry->setMRU();
}
