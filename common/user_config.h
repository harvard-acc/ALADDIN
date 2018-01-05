#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#include "AladdinExceptions.h"
#include "ExecNode.h"
#include "SourceEntity.h"
#include "typedefs.h"

enum MemoryType {
  spad,
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

  unrolling_config_t unrolling;
  pipeline_config_t pipeline;
  partition_config_t partition;

  float cycle_time;
  bool ready_mode;
  unsigned scratchpad_ports;
  bool global_pipelining;
};


#endif
