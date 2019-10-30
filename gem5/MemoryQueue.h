#ifndef __MEMORY_QUEUE_H__
#define __MEMORY_QUEUE_H__

#include <string>

#include "base/flags.hh"
#include "base/statistics.hh"
#include "sim/core.hh"

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

/* All possible types of memory operations supported by Aladdin. */
enum MemoryOpType {
  Register,
  Scratchpad,
  Cache,
  Dma,
  ACP,
  ReadyBits,
  NumMemoryOpTypes,
};

class MemoryQueueEntry {
 public:
  MemoryQueueEntry() {
    status = Invalid;
    vaddr = 0x0;
    paddr = 0x0;
    size = 0;
    type = NumMemoryOpTypes;
    when_allocated = curTick();
    when_issued = 0;
  }

  Tick when_allocated;
  Tick when_issued;
  MemoryOpType type;
  MemAccessStatus status;  // Current status of the request.
  Addr vaddr;              // Virtual address, returned by the TLB.
  Addr paddr;              // Physical address, returned by the TLB.
  int size;
  bool isLoad;
};

class MemoryQueue {
 public:
  MemoryQueue(int _size,
              int _bandwidth,
              int _cache_line_size,
              std::string _cacti_config)
      : issuedThisCycle(0), size(_size), bandwidth(_bandwidth),
        cache_line_size(_cache_line_size), cacti_config(_cacti_config),
        readEnergy(0), writeEnergy(0), leakagePower(0), area(0) {}

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

  bool isFull() {
    if (ops.size() >= size) {
      warn("The memory queue becomes full. Capacity: %d, current size: %d.\n",
           size,
           ops.size());
    }
    return false;
  }

  /* Returns true if the ops already contains an entry for this address. */
  bool contains(Addr vaddr) { return (ops.find(vaddr) != ops.end()); }

  bool insert(Addr vaddr) {
    if (!isFull() && !contains(vaddr)) {
      ops[vaddr] = new MemoryQueueEntry();
      return true;
    }
    return false;
  }

  MemoryQueueEntry* findMatch(Addr vaddr, bool merge) {
    vaddr = merge ? toCacheLineAddr(vaddr) : vaddr;
    return findMatch(vaddr);
  }

  MemoryQueueEntry* allocateEntry(Addr vaddr, int size, bool merge) {
    if (!isFull()) {
      vaddr = merge ? toCacheLineAddr(vaddr) : vaddr;
      MemoryQueueEntry* entry = new MemoryQueueEntry();
      entry->vaddr = vaddr;
      entry->size = size;
      entry->isLoad = merge;
      ops[vaddr] = entry;
      return entry;
    }
    return nullptr;
  }

  void deallocateEntry(Addr vaddr, bool merge) {
    vaddr = merge ? toCacheLineAddr(vaddr) : vaddr;
    remove(vaddr);
  }

  void remove(Addr vaddr) {
    delete ops[vaddr];
    ops.erase(vaddr);
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
      MemoryQueueEntry* entry = it->second;
      if (entry->status == Returned) {
        ops.erase(it++);  // Must be post-increment!
        delete entry;
      } else {
        ++it;
      }
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
      std::cout << "0x" << it->first << ": " << it->second->status << "\n";
    }
    std::cout << std::dec;
  }

 protected:
  MemoryQueueEntry* findMatch(Addr vaddr) {
    if (ops.find(vaddr) != ops.end())
      return ops[vaddr];
    return nullptr;
  }

  Addr toCacheLineAddr(Addr addr) {
    return (addr / cache_line_size) * cache_line_size;
  }

 private:
  const int size;         // Size of the queue.
  const int bandwidth;    // Max requests per cycle.
  const int cache_line_size;
  const std::string cacti_config;  // CACTI config file.
  std::string name;  // Specifies whether this is a load or store queue.

  float readEnergy;
  float writeEnergy;
  float leakagePower;
  float area;

  Stats::Scalar readStats;
  Stats::Scalar writeStats;
  size_t issuedThisCycle;  // Requests issued in the current cycle.

  // Maps address to the associated memory operation status.
  std::map<Addr, MemoryQueueEntry*> ops;
};

#endif
