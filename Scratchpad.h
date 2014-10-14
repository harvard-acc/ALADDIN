#ifndef __SCRATCHPAD__
#define __SCRATCHPAD__

#include <assert.h>
#include <iostream>
#include <math.h>
#include <unordered_map>
#include <map>
#include <vector>

#include "power_delay.h"
#include "MemoryInterface.h"

class Scratchpad : public MemoryInterface
{
 public:
  Scratchpad(unsigned p_ports_per_part);
  ~Scratchpad();
  void step();
  void setScratchpad(std::string baseName, unsigned size);
  void setCompScratchpad(std::string baseName, unsigned size);
  bool canService();
  bool canServicePartition(std::string baseName);
  bool partitionExist(std::string baseName);
  unsigned findPartitionID(std::string baseName);
  bool addressRequest(std::string baseName);

  /* Increment the loads counter for the specified partition. */
  void increment_loads(std::string partition);
  /* Increment the stores counter for the specified partition. */
  void increment_stores(std::string partition);

  void getMemoryBlocks(std::vector<std::string> &names);
  void getRegisterBlocks(std::vector<std::string> &names);
  float getAveragePower(unsigned int cycles);
  float getTotalArea();
  float getReadPower(std::string baseName);
  float getWritePower(std::string baseName);
  float getLeakagePower(std::string baseName);
  float getArea(std::string baseName);

private:
  unsigned numOfPartitions;
  unsigned numOfPortsPerPartition;
  std::unordered_map<std::string, unsigned> baseToPartitionID;
  /* Number of loads per partition. */
  std::map<std::string, unsigned> partition_loads;
  /* Number of stores per partition. */
  std::map<std::string, unsigned> partition_stores;

  std::vector<bool> compPartition;
  std::vector<unsigned> occupiedBWPerPartition;
  std::vector<unsigned> sizePerPartition;
  std::vector<float> readPowerPerPartition;
  std::vector<float> writePowerPerPartition;
  std::vector<float> leakPowerPerPartition;
  std::vector<float> areaPerPartition;
};

#endif
