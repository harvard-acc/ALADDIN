#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#include <iostream>

#include "AladdinExceptions.h"
#include "ExecNode.h"
#include "MemoryType.h"
#include "SourceEntity.h"
#include "typedefs.h"

// None is used if an array is stored in a cache.
enum PartitionType { block, cyclic, complete, none };

struct PartitionEntry {
  MemoryType memory_type;
  PartitionType partition_type;
  unsigned array_size;  // num of bytes
  unsigned wordsize;    // in bytes
  unsigned part_factor;
  Addr base_addr;
};

class UserConfigParams {
 public:
  UserConfigParams()
      : cycle_time(1), ready_mode(false), scratchpad_ports(1),
        global_pipelining(false) {}

  partition_config_t::const_iterator getArrayConfig(Addr addr) const {
    auto part_it = partition.begin();
    for (; part_it != partition.end(); ++part_it) {
      Addr base = part_it->second.base_addr;
      size_t size = part_it->second.array_size;
      if (base <= addr && base + size > addr)
        break;
    }

    if (part_it == partition.end() || part_it->first.empty()) {
      throw UnknownArrayException(addr);
    }
    return part_it;
  }

  const PartitionEntry& getArrayConfig(const std::string& array_name) {
    auto part_it = partition.find(array_name);
    if (part_it == partition.end())
      throw UnknownArrayException(array_name);
    return part_it->second;
  }

  void updateArrayParams(const std::string& array_name,
                         unsigned size) {
    auto part_it = partition.find(array_name);
    Addr curr_array_base = 0;
    if (part_it != partition.end()) {
      MemoryType mtype = part_it->second.memory_type;
      if (mtype == cache || mtype == acp || mtype == dma) {
        part_it->second.array_size = size;
        curr_array_base = part_it->second.base_addr;
      } else if (size != part_it->second.array_size) {
        std::cerr << "[WARNING]: " << array_name
                  << " is mapped to a scratchpad or "
                     "register file, whose size cannot "
                     "be changed dynamically!\n";
        return;
      }
    } else {
      // The only case in where we don't find this array is if when we get a
      // call to mapArrayToAccelerator() for a region of host memory. We won't
      // have the trace base address until we parse the entry block in the
      // dynamic trace, but we can still set the size now.
      // By default, the host memory type is set to DMA.
      partition[array_name] = { dma, none, size, 0, 0, 0 };
    }
    // To resolve the overlapping array case, find all other partition entries
    // that overlap with this range and set their sizes to zero, so we can
    // guarantee that a call to getArrayConfig(addr) will not return the wrong
    // array. This only applies for the types of memory that can be
    // configurable at runtime (host/acp/cache).
    for (auto part_it = partition.begin(); part_it != partition.end();
         ++part_it) {
      const std::string& part_name = part_it->first;
      Addr part_base = part_it->second.base_addr;
      size_t part_size = part_it->second.array_size;
      MemoryType mtype = part_it->second.memory_type;
      if ((mtype == cache || mtype == acp || mtype == dma) &&
          part_name != array_name &&
          rangesOverlap(curr_array_base,
                        curr_array_base + size,
                        part_base,
                        part_base + part_size)) {
        part_it->second.array_size = 0;
      }
    }
  }

  // For each array we know of, check if the address ranges overlap.
  //
  // When overlaps are found, print the overlapping arrays and their address
  // ranges. Overlapping ranges almost always lead to incorrect behavior.
  void checkOverlappingRanges() {
    std::set<OverlappingArrayPair> overlapping_arrays;
    for (auto& array_0 : partition) {
      // Compare each array with all the others.
      const std::string& array_0_name = array_0.first;
      Addr base_0 = array_0.second.base_addr;
      unsigned size_0 = array_0.second.array_size;
      Addr end_0 = base_0 + size_0;

      if (base_0 == 0)
        continue;
      for (auto& array_1 : partition) {
        const std::string& array_1_name = array_1.first;
        if (array_0_name == array_1_name)
          continue;
        Addr base_1 = array_1.second.base_addr;
        unsigned size_1 = array_1.second.array_size;
        unsigned end_1 = base_1 + size_1;
        if (base_1 == 0)
          continue;

        if (rangesOverlap(base_0, end_0, base_1, end_1)) {
          continue;
        } else {
          overlapping_arrays.insert(OverlappingArrayPair(
              array_0_name, base_0, end_0, array_1_name, base_1, end_1));
        }
      }
    }
    if (!overlapping_arrays.empty()) {
      for (auto& array_pair : overlapping_arrays) {
        std::cerr << array_pair << "\n";
      }
      std::cerr
          << "Overlapping array address ranges can lead to incorrect behavior, "
             "such as DMA nodes trying to access the wrong arrays, ACP "
             "accessing the wrong memory, etc. Please check your Aladdin "
             "configuration file and the mapArrayToAccelerator() calls to "
             "verify that they are correct.\n";
    }
  }

  unrolling_config_t unrolling;
  pipeline_config_t pipeline;
  partition_config_t partition;

  float cycle_time;
  bool ready_mode;
  unsigned scratchpad_ports;
  bool global_pipelining;

 protected:
  // Returns true if the ranges defined by [base_0, end_0) and [base_1, end_1)
  // overlap.
  bool rangesOverlap(Addr base_0, Addr end_0, Addr base_1, Addr end_1) {
    // Two ranges [A,B], [C,D] do not overlap if A and B are both less than
    // C or both greater than D.
    bool disjoint = ((base_0 < base_1 && end_0 <= base_1) ||
                     (base_0 >= end_1 && end_0 > end_1));
    return !disjoint;
  }

  // This class defines a pair of arrays that overlap.
  class OverlappingArrayPair {
   protected:
    // An internal description of an array.
    struct Array {
     public:
      Array(const std::string& _name, Addr _base, Addr _end)
          : name(_name), base(_base), end(_end) {}

      bool operator==(const Array& other) const {
        return name == other.name && base == other.base && end == other.end;
      }

      friend std::ostream& operator<<(std::ostream& os,
                                      const Array& obj) {
        os << obj.name << ": " << std::hex << "0x" << obj.base << " - 0x"
           << obj.end << std::dec;
        return os;
      }

      std::string name;
      Addr base;
      Addr end;
    };

    Array array0;
    Array array1;

   public:
    OverlappingArrayPair(const std::string& array_0_name,
                         Addr base_0,
                         Addr end_0,
                         const std::string& array_1_name,
                         Addr base_1,
                         Addr end_1)
        : array0(array_0_name, base_0, end_0),
          array1(array_1_name, base_1, end_1) {}

    // Two OverlappingArrayPair objects are equal if their arrays are equal, in
    // any order.
    bool operator==(const OverlappingArrayPair& other) const {
      return (array0 == other.array0 && array1 == other.array1) ||
             (array0 == other.array1 && array1 == other.array0);
    }

    bool operator<(const OverlappingArrayPair& other) const {
      return (array0.base < array1.base);
    }

    friend std::ostream& operator<<(std::ostream& os,
                                    const OverlappingArrayPair& obj) {
      os << "[WARNING]: Overlapping array declarations found!\n"
         << "  " << obj.array0 << "\n  " << obj.array1;
      return os;
    }
  };
};


#endif
