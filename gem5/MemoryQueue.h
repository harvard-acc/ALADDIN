#ifndef __MEMORY_QUEUE_H__
#define __MEMORY_QUEUE_H__

#include <string>

#include "base/flags.hh"
#include "base/statistics.hh"

// Current status of a memory access. Used for caches and DMA requests.
enum MemAccessStatus {
  Ready,
  Translating,
  Translated,
  WaitingFromCache,
  WaitingFromDma,
  WaitingForDmaSetup,
  Returned
};

struct MemoryQueueEntry {
  MemAccessStatus status;  // Current status of the request.
  Addr paddr;              // Physical address, returned by the TLB.

  MemoryQueueEntry() {
    status = Ready;
    paddr = 0x0;
  }
};

/* TODO: Rename this to something more suitable to reflect that it is just
 * a simulation bookkeeping structure.
 */
class MemoryQueue {
 public:
  MemoryQueue() {}

  /* Returns true if the ops already contains an entry for this address. */
  bool contains(Addr vaddr) { return (ops.find(vaddr) != ops.end()); }

  bool insert(Addr vaddr) {
    if (!contains(vaddr)) {
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
    ops[vaddr].paddr = paddr;
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

 private:
  // Maps address to the associated memory operation status.
  std::map<Addr, MemoryQueueEntry> ops;
};

#endif
