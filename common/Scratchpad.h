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

/* Use <number of bytes, word size> as a unique key for cacti */
typedef std::pair<unsigned, unsigned> cacti_key;
class Scratchpad {
 public:
  Scratchpad(unsigned p_ports_per_part, float cycle_time);
  virtual ~Scratchpad();
  void step();
  void setScratchpad(std::string baseName,
                     unsigned num_of_bytes,
                     unsigned wordsize);
  bool canService();
  bool canServicePartition(std::string baseName);
  bool partitionExist(std::string baseName);
  unsigned findPartitionID(std::string baseName);
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
  std::unordered_map<std::string, unsigned> baseToPartitionID;
  /* Occupied BW  per partition. */
  std::unordered_map<std::string, unsigned> occupiedBWPerPartition;
  /* Number of loads per partition. */
  std::map<std::string, unsigned> partition_loads;
  /* Number of stores per partition. */
  std::map<std::string, unsigned> partition_stores;
  std::map<cacti_key, uca_org_t> cacti_value;

  std::vector<bool> compPartition;
  std::vector<unsigned> sizePerPartition;
  std::vector<float> readEnergyPerPartition;
  std::vector<float> writeEnergyPerPartition;
  std::vector<float> leakPowerPerPartition;
  std::vector<float> areaPerPartition;
};

#endif
