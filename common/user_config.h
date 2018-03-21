#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#include <iostream>

#include "AladdinExceptions.h"
#include "ExecNode.h"
#include "SourceEntity.h"
#include "typedefs.h"

enum MemoryType {
  spad,
  spad_bypass,
  reg,
  cache,
  acp,
  host,
};

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

// Every mapping entry stores an address range mapping.
struct SpadToDmaMappingEntry {
  Addr spad_start;
  Addr host_start;
  std::string host_array_name;
  unsigned size;
};

// This class maintains all the mappings of scratchpad address ranges to host
// memory address ranges in a scratchpad.
// For example, a dmaLoad(spad+100, host+200, 300) would add a new mapping entry
// which maps the address range of {spad+100, spad+400} in spad to host address
// range {host+200, host+500}.
class SpadToDmaMapping {
 public:
  SpadToDmaMapping() : part_entry(nullptr) { mapping_entries.clear(); }
  SpadToDmaMapping(PartitionEntry* _part_entry) : part_entry(_part_entry) {
    mapping_entries.clear();
  }

  std::pair<std::string, Addr> getSpadToDmaMapping(Addr spad_addr) {
    for (auto& entry : mapping_entries) {
      if (entry.spad_start <= spad_addr &&
          entry.spad_start + entry.size > spad_addr) {
        return { entry.host_array_name,
                 entry.host_start + spad_addr - entry.spad_start };
      }
    }
    std::cerr << "Do not find host array mapping for the scratchpad address!\n";
    exit(1);
  }

  void updateSpadToDmaMapping(Addr spad_start,
                              Addr host_start,
                              std::string host_array_name,
                              unsigned size) {
    // Check if the new mapping overlaps with an existing one. If so, replace
    // the old one with the new one.
    for (auto it = mapping_entries.begin(); it != mapping_entries.end();) {
      if ((it->spad_start <= spad_start &&
           it->spad_start + it->size > spad_start) ||
          (it->spad_start > spad_start &&
           it->spad_start + it->size <= spad_start + size)) {
        it = mapping_entries.erase(it);
      } else {
        ++it;
      }
    }
    mapping_entries.push_back({ spad_start, host_start, host_array_name, size });
  }

  PartitionEntry* part_entry;
  std::vector<SpadToDmaMappingEntry> mapping_entries;
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

  void setArraySize(const std::string& array_name, unsigned size) {
    auto part_it = partition.find(array_name);
    if (part_it != partition.end()) {
      MemoryType mtype = part_it->second.memory_type;
      if (mtype == cache || mtype == acp || mtype == host) {
        part_it->second.array_size = size;
      } else {
        if (size != part_it->second.array_size) {
          std::cerr << "[WARNING]: " << array_name
                    << " is mapped to a scratchpad or "
                       "register file, whose size cannot "
                       "be changed dynamically!\n";
        }
      }
    } else {
      // The only case in where we don't find this array is if when we get a
      // call to mapArrayToAccelerator() for a region of host memory. We won't
      // have the trace base address until we parse the entry block in the
      // dynamic trace, but we can still set the size now.
      partition[array_name] = { host, none, size, 0, 0, 0 };
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

        // Two ranges [A,B], [C,D] do not overlap if A and B are both less than
        // C or both greater than D.
        if ((base_0 < base_1 && end_0 <= base_1) ||
            (base_0 >= end_1 && end_0 > end_1)) {
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

  void updateSpadToDmaMapping(std::string spad_name,
                              std::string host_array_name,
                              Addr spad_start,
                              Addr host_start,
                              unsigned size) {
    std::cout << "Updating scratchpad to host array mappings. Scratchpad name: "
              << spad_name << ", spad base: " << std::hex << spad_start
              << ", host array name: " << host_array_name
              << ", host base: " << host_start << std::dec << ", size " << size
              << "\n";
    auto map_it = spad_dma_mapping.find(spad_name);
    if (map_it != spad_dma_mapping.end()) {
      map_it->second.updateSpadToDmaMapping(
          spad_start, host_start, host_array_name, size);
    } else {
      auto part_it = partition.find(spad_name);
      if (part_it != partition.end()) {
        spad_dma_mapping[spad_name] = SpadToDmaMapping(&(part_it->second));
        spad_dma_mapping[spad_name].updateSpadToDmaMapping(
            spad_start, host_start, host_array_name, size);
      } else {
        std::cerr << "No partition entry is found for the scratchpad name "
                  << spad_name << "!\n";
        exit(1);
      }
    }
  }

  std::pair<std::string, Addr> getSpadToDmaMapping(std::string spad_name,
                                                   Addr spad_addr) {
    auto map_it = spad_dma_mapping.find(spad_name);
    if (map_it != spad_dma_mapping.end()) {
      return map_it->second.getSpadToDmaMapping(spad_addr);
    }
    std::cerr << spad_name
              << " does not have scrachpad to any host memory mappings!\n";
    exit(1);
  }

  unrolling_config_t unrolling;
  pipeline_config_t pipeline;
  partition_config_t partition;
  spad_dma_mapping_t spad_dma_mapping;

  float cycle_time;
  bool ready_mode;
  unsigned scratchpad_ports;
  bool global_pipelining;

 protected:
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
         << "  " << obj.array0 << "\n" << obj.array1;
      return os;
    }
  };
};


#endif
