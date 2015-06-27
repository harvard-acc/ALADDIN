#ifndef __SCRATCHPAD__
#define __SCRATCHPAD__

#include <assert.h>
#include <iostream>
#include <math.h>
#include <unordered_map>
#include <map>
#include <vector>

#include "power_func.h"
#include "cacti-p/io.h"
#include "cacti-p/cacti_interface.h"

struct partition_t {
  unsigned occupied_bw;
  unsigned loads;
  unsigned stores;
  unsigned size;
  float read_energy;
  float write_energy;
  float leak_power;
  float area;
  partition_t() {
    occupied_bw = 0;
    loads = 0;
    stores = 0;
    size = 0;
    read_energy = 0;
    write_energy = 0;
    leak_power = 0;
    area = 0;
  }
};
/* Use <number of bytes, word size> as a unique key for cacti */
typedef std::pair<unsigned, unsigned> cacti_key;
class Scratchpad {
 public:
  Scratchpad(unsigned p_ports_per_part, float cycle_time);
  virtual ~Scratchpad();
  void clear();
  void step();
  void setScratchpad(std::string baseName,
                     unsigned num_of_bytes,
                     unsigned wordsize);
  bool canService();
  bool canServicePartition(std::string baseName);
  bool partitionExist(std::string baseName);
  bool addressRequest(std::string baseName);

  /* Increment the loads counter for the specified partition. */
  void increment_loads(std::string partition);
  /* Increment the stores counter for the specified partition. */
  void increment_stores(std::string partition);

  void getMemoryBlocks(std::vector<std::string>& names);

  void getAveragePower(unsigned int cycles,
                       float* avg_power,
                       float* avg_dynamic,
                       float* avg_leakage);
  float getTotalArea();
  unsigned getTotalSize();
  float getReadEnergy(std::string baseName);
  float getWriteEnergy(std::string baseName);
  float getLeakagePower(std::string baseName);
  float getArea(std::string baseName);

  /* Access cacti_interface() to calculate power/area. */
  uca_org_t cactiWrapper(unsigned num_of_bytes, unsigned wordsize);

 private:
  unsigned numOfPartitions;
  unsigned numOfPortsPerPartition;
  float cycleTime;  // in ns
  std::unordered_map<std::string, partition_t> partitions;

  std::map<cacti_key, uca_org_t> cacti_value;
};

#endif
