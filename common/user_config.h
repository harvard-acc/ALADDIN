#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

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
    // If the array label is not found, abort the simulation.
    // const std::string& array_label = part_it->first;
    if (part_it == partition.end() || part_it->first.empty()) {
      std::cerr << "Cannot find array with address : 0x" << std::hex << addr
                << std::endl;
      assert(false);
    }
    return part_it;
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
