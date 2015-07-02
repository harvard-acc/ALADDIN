#ifndef __MEMORY_QUEUE_H__
#define __MEMORY_QUEUE_H__

#include <string>

#include "base/flags.hh"
#include "base/statistics.hh"

#include "aladdin/common/cacti-p/cacti_interface.h"
#include "aladdin/common/cacti-p/io.h"

/*hack, to fix*/
#define MIN_CACTI_SIZE 64

// Current status of a memory access. Used for caches and DMA requests.
enum MemAccessStatus {
  Ready,
  Translating,
  Translated,
  WaitingFromCache,
  WaitingFromDma,
  Returned
};

struct MemoryQueueEntry {
  MemAccessStatus status;  // Current status of the request.
  Addr paddr;  // Physical address, returned by the TLB.

  MemoryQueueEntry() {
    status = Ready;
    paddr = 0x0;
  }
};

/* A generic memory queue that stores the number of reads and writes from some
 * shared memory resource.
 */
class MemoryQueue {
 public:
  MemoryQueue(int _size,
              int _bandwidth,
              std::string _name,
              std::string _cacti_config)
      : size(_size), bandwidth(_bandwidth), issued_this_cycle(0),
        name(_name), cacti_config(_cacti_config), readEnergy(0), writeEnergy(0),
        leakagePower(0), area(0) {
    readStats.name(name + "_reads")
        .desc("Number of reads to the " + name)
        .flags(Stats::total | Stats::nonan);
    writeStats.name(name + "_writes")
        .desc("Number of writes to the " + name)
        .flags(Stats::total | Stats::nonan);
  }

  /* Returns true if we have not exceeded the cache's bandwidth.
   */
  bool can_issue() {
    return (issued_this_cycle < bandwidth);
  }

  /* Returns true if the queue already contains an entry for this address. */
  bool contains(Addr vaddr) {
    return (queue.find(vaddr) != queue.end());
  }

  bool is_full() {
    return (queue.size() == size);
  }

  bool enqueue(Addr vaddr) {
    if (!contains(vaddr) && !is_full()) {
      queue[vaddr] = MemoryQueueEntry();
      return true;
    }
    return false;
  }

  void dequeue(Addr vaddr) {
    queue.erase(vaddr);
  }

  void setStatus(Addr vaddr, MemAccessStatus status) {
    assert(contains(vaddr));
    queue[vaddr].status = status;
  }

  MemAccessStatus getStatus(Addr vaddr) {
    return queue[vaddr].status;
  }

  void setPhysicalAddress(Addr vaddr, Addr paddr) {
    queue[vaddr].paddr = paddr;
  }

  /* Retires all entries that are returned.
   *
   * This is done at the end of every cycle to free space for new requests.
   */
  void retireReturnedEntries() {

    for (auto it = queue.begin(); it != queue.end(); /* no increment */) {
      if (it->second.status == Returned)
        queue.erase(it++);  // Must be post-increment!
      else
        ++it;
    }
  }

  void computeCactiResults() {
    uca_org_t cacti_result = cacti_interface(cacti_config);
    if (size >= MIN_CACTI_SIZE) {
      readEnergy = cacti_result.power.readOp.dynamic * 1e9;
      writeEnergy = cacti_result.power.writeOp.dynamic * 1e9;
      leakagePower = cacti_result.power.readOp.leakage * 1000;
      area = cacti_result.area;
    } else {
      /*Assuming it scales linearly with cache size*/
      readEnergy =
          cacti_result.power.readOp.dynamic * 1e9 * size / MIN_CACTI_SIZE;
      writeEnergy =
          cacti_result.power.writeOp.dynamic * 1e9 * size / MIN_CACTI_SIZE;
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
  float getArea() { return area; }

  const int size;         // Size of the queue.
  const int bandwidth;    // Max requests per cycle.
  int issued_this_cycle;  // Requests issued in the current cycle.
  Stats::Scalar readStats;
  Stats::Scalar writeStats;

 private:
  std::string name;          // Specifies whether this is a load or store queue.
  std::string cacti_config;  // CACTI config file.
  float readEnergy;
  float writeEnergy;
  float leakagePower;
  float area;

  // The actual "queue". We can use a map here because it's actually the
  // Aladdin scheduler that determines which memory node gets executed, not the
  // position of the memory access in the queue.
  std::map<Addr, MemoryQueueEntry> queue;
};

#endif
