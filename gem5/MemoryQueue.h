#ifndef __MEMORY_QUEUE_H__
#define __MEMORY_QUEUE_H__

#include <string>

#include "base/flags.hh"
#include "base/statistics.hh"

#define MIN_CACTI_SIZE 64

// Current status of a memory access. Used for caches and DMA requests.
enum MemAccessStatus {
  Invalid,
  Retry,
  Translating,
  Translated,
  ReadyToRequestOwnership,
  ReadyToIssue,
  WaitingForOwnership,
  WaitingFromCache,
  WaitingFromDma,
  WaitingForDmaSetup,
  Returned
};

struct MemoryQueueEntry {
  MemAccessStatus status;  // Current status of the request.
  Addr paddr;              // Physical address, returned by the TLB.

  MemoryQueueEntry() {
    status = Invalid;
    paddr = 0x0;
  }
};

class MemoryQueue {
 public:
  MemoryQueue(int _size, int _bandwidth, std::string _cacti_config)
      : issuedThisCycle(0), size(_size), bandwidth(_bandwidth),
        cacti_config(_cacti_config), readEnergy(0), writeEnergy(0),
        leakagePower(0), area(0) {}

  void initStats(std::string _name) {
    name = _name;
    readStats.name("system." + name + "_reads")
        .desc("Number of reads to the " + name)
        .flags(Stats::total | Stats::nonan);
    writeStats.name("system." + name + "_writes")
        .desc("Number of writes to the " + name)
        .flags(Stats::total | Stats::nonan);
  }

  bool canIssue() { return bandwidth == 0 ? true : (issuedThisCycle < bandwidth); }

  bool isFull() { return ops.size() == size; }

  /* Returns true if the ops already contains an entry for this address. */
  bool contains(Addr vaddr) { return (ops.find(vaddr) != ops.end()); }

  bool insert(Addr vaddr) {
    if (!isFull() && !contains(vaddr)) {
      ops[vaddr] = MemoryQueueEntry();
      return true;
    }
    return false;
  }

  void remove(Addr vaddr) { ops.erase(vaddr); }

  void setStatus(Addr vaddr, MemAccessStatus status) {
    assert(contains(vaddr));
    ops[vaddr].status = status;
  }

  MemAccessStatus getStatus(Addr vaddr) { return ops[vaddr].status; }

  void setPhysicalAddress(Addr vaddr, Addr paddr) {
    assert(contains(vaddr));
    ops[vaddr].paddr = paddr;
  }

  void incrementIssuedThisCycle() {
    issuedThisCycle ++;
  }

  void resetIssuedThisCycle() {
    issuedThisCycle = 0;
  }

  /* Retires all entries that are returned.
   *
   * This is done at the end of every cycle to free space for new requests.
   */
  void retireReturnedEntries() {
    for (auto it = ops.begin(); it != ops.end(); /* no increment */) {
      if (it->second.status == Returned)
        ops.erase(it++);  // Must be post-increment!
      else
        ++it;
    }
  }

  void computeCactiResults() {
    uca_org_t cacti_result = cacti_interface(cacti_config);
    if (size >= MIN_CACTI_SIZE) {
      readEnergy = cacti_result.power.readOp.dynamic * 1e12;
      writeEnergy = cacti_result.power.writeOp.dynamic * 1e12;
      leakagePower = cacti_result.power.readOp.leakage * 1000;
      area = cacti_result.area;
    } else {
      /*Assuming it scales linearly with cache size*/
      readEnergy =
          cacti_result.power.readOp.dynamic * 1e12 * size / MIN_CACTI_SIZE;
      writeEnergy =
          cacti_result.power.writeOp.dynamic * 1e12 * size / MIN_CACTI_SIZE;
      leakagePower =
          cacti_result.power.readOp.leakage * 1000 * size / MIN_CACTI_SIZE;
      area = cacti_result.area * size / MIN_CACTI_SIZE;
    }
  }

  void getAveragePower(unsigned int cycles,
                       unsigned int cycleTime,
                       float* avg_power,
                       float* avg_dynamic,
                       float* avg_leak) {
    *avg_dynamic =
        (readStats.value() * readEnergy + writeStats.value() * writeEnergy) /
        (cycles * cycleTime);
    *avg_leak = leakagePower;
    *avg_power = *avg_dynamic + *avg_leak;
  }

  void resetCounters() {
    readStats = 0;
    writeStats = 0;
  }

  float getArea() { return area; }

  void printContents() {
    std::cout << std::hex;
    for (auto it = ops.begin(); it != ops.end(); it++) {
      std::cout << "0x" << it->first << ": " << it->second.status << "\n";
    }
    std::cout << std::dec;
  }

  Stats::Scalar readStats;
  Stats::Scalar writeStats;
  size_t issuedThisCycle;  // Requests issued in the current cycle.

 private:
  const int size;         // Size of the queue.
  const int bandwidth;    // Max requests per cycle.
  const std::string cacti_config;  // CACTI config file.
  std::string name;  // Specifies whether this is a load or store queue.

  float readEnergy;
  float writeEnergy;
  float leakagePower;
  float area;

  // Maps address to the associated memory operation status.
  std::map<Addr, MemoryQueueEntry> ops;
};

#endif
