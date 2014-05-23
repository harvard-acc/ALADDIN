#include "./Scratchpad.h"
Scratchpad::Scratchpad(float p_latency, unsigned p_ports_per_part)
{
  latency = p_latency;
  numOfPortsPerPartition = p_ports_per_part;
}

void Scratchpad::setScratchpad(string baseName, unsigned size)
{
  assert(!partitionExist(baseName));
  
  unsigned new_id = baseToPartitionID.size();
  baseToPartitionID[baseName] = new_id;
  sizePerPartition.push_back(size);
  occupiedBWPerPartition.push_back(0);
}

void Scratchpad::step()
{
  for (auto it = occupiedBWPerPartition.begin(); it != occupiedBWPerPartition.end(); ++it)
    *it = 0;
}
bool Scratchpad::partitionExist(string baseName)
{
  auto partition_it = baseToPartitionID.find(baseName);
  if (partition_it != baseToPartitionID.end())
    return 1;
  else
    return 0;
}
float Scratchpad::addressRequest(string baseName)
{
  if (canServicePartition(baseName))
  {
    unsigned partition_id = findPartitionID(baseName);
    assert(occupiedBWPerPartition.at(partition_id) < numOfPortsPerPartition);
    occupiedBWPerPartition.at(partition_id)++;
    return latency;
  }
  else
    return -1;
}
bool Scratchpad::canService()
{
  for(auto base_it = baseToPartitionID.begin(); base_it != baseToPartitionID.end(); ++base_it)
  {
    string base_name = base_it->first;
    if (canServicePartition(base_name))
      return 1;
  }
  return 0;
}
bool Scratchpad::canServicePartition(string baseName)
{
  unsigned partition_id = findPartitionID(baseName);
  if (occupiedBWPerPartition.at(partition_id) < numOfPortsPerPartition)
    return 1;
  else
    return 0;
}

unsigned Scratchpad::findPartitionID(string baseName)
{
  auto partition_it = baseToPartitionID.find(baseName);
  if (partition_it != baseToPartitionID.end())
    return partition_it->second;
  else
  {
    cerr << "Unknown Partition Name:" << baseName << endl;
    exit(0);
  }
}

void Scratchpad::partitionNames(std::vector<string> &names)
{
  for(auto base_it = baseToPartitionID.begin(); base_it != baseToPartitionID.end(); ++base_it)
  {
    string base_name = base_it->first;
    names.push_back(base_name);
  }
}
