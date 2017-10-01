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
};

// None is used if an array is stored in a cache.
enum PartitionType { block, cyclic, complete, none };

struct PartitionEntry {
  MemoryType memory_type;
  PartitionType partition_type;
  unsigned array_size;  // num of bytes
  unsigned wordsize;    // in bytes
  unsigned part_factor;
  long long int base_addr;
};

class UserConfigParams {
 public:
  UserConfigParams()
      : cycle_time(1), ready_mode(false), scratchpad_ports(1),
        global_pipelining(false) {}

  unrolling_config_t unrolling;
  pipeline_config_t pipeline;
  partition_config_t partition;

  float cycle_time;
  bool ready_mode;
  unsigned scratchpad_ports;
  bool global_pipelining;
};


#endif
