#ifndef __SCRATCHPAD__
#define __SCRATCHPAD__

#include <iostream>
#include <assert.h>
#include <unordered_map>
#include <vector>
#include "./Datapath.h"


class Scratchpad
{
 public:
  Scratchpad(float p_latency, unsigned p_ports_per_part);
  ~Scratchpad();
  void step();
  void setScratchpad(string baseName, unsigned size);
  bool canService();
  bool canServicePartition(string baseName);
  bool partitionExist(string baseName);
  unsigned findPartitionID(string baseName);
  float addressRequest(string baseName);

 private:
  unsigned numOfPartitions;
  unsigned numOfPortsPerPartition;
  unordered_map<string, unsigned> baseToPartitionID;
  vector<unsigned> occupiedBWPerPartition;
  vector<unsigned> sizePerPartition;
  float latency;

};
#endif
