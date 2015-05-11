#include "generic_func.h"
#include "Scratchpad.h"

Scratchpad::Scratchpad(unsigned p_ports_per_part, float cycle_time) {
  numOfPortsPerPartition = p_ports_per_part;
  cycleTime = cycle_time;
}

Scratchpad::~Scratchpad() {}

void Scratchpad::setScratchpad(std::string baseName,
                               unsigned num_of_bytes,
                               unsigned wordsize,
                               uca_org_t cacti_result)
    // wordsize in bytes
{
  assert(!partitionExist(baseName));
  // size: number of words
  unsigned new_id = baseToPartitionID.size();
  baseToPartitionID[baseName] = new_id;

  compPartition.push_back(false);
  occupiedBWPerPartition[baseName] = 0;
  sizePerPartition.push_back(num_of_bytes);
  // set read/write/leak/area per partition

  // power in mW, energy in nJ, area in mm2
  readEnergyPerPartition.push_back(cacti_result.power.readOp.dynamic * 1e+9);
  writeEnergyPerPartition.push_back(cacti_result.power.writeOp.dynamic * 1e+9);
  leakPowerPerPartition.push_back(cacti_result.power.readOp.leakage * 1000);
  areaPerPartition.push_back(cacti_result.area);
}

void Scratchpad::step() {
  for (auto it = occupiedBWPerPartition.begin();
       it != occupiedBWPerPartition.end();
       ++it) {
    it->second = 0;
  }
}
bool Scratchpad::partitionExist(std::string baseName) {
  auto partition_it = baseToPartitionID.find(baseName);
  if (partition_it != baseToPartitionID.end())
    return true;
  else
    return false;
}
bool Scratchpad::addressRequest(std::string baseName) {
  if (canServicePartition(baseName)) {
    assert(occupiedBWPerPartition[baseName] < numOfPortsPerPartition);
    occupiedBWPerPartition[baseName]++;
    return true;
  } else
    return false;
}
bool Scratchpad::canService() {
  for (auto base_it = occupiedBWPerPartition.begin();
       base_it != occupiedBWPerPartition.end();
       ++base_it) {
    if (canServicePartition(base_it->first))
      return true;
  }
  return false;
}
bool Scratchpad::canServicePartition(std::string baseName) {
  if (occupiedBWPerPartition[baseName] < numOfPortsPerPartition)
    return true;
  else
    return false;
}

unsigned Scratchpad::findPartitionID(std::string baseName) {

  auto partition_it = baseToPartitionID.find(baseName);
  if (partition_it != baseToPartitionID.end())
    return partition_it->second;
  else {
    std::cerr << "Unknown Partition Name:" << baseName << std::endl;
    std::cerr << "Need to Explicitly Declare How to Partition Each Array in "
                 "the Config File" << std::endl;
    exit(0);
  }
}
// power in mW, energy in nJ, time in ns
void Scratchpad::getAveragePower(unsigned int cycles,
                                 float* avg_power,
                                 float* avg_dynamic,
                                 float* avg_leak) {
  float load_energy = 0;
  float store_energy = 0;
  float leakage_power = 0;
  for (auto it = partition_loads.begin(); it != partition_loads.end(); ++it)
    load_energy += it->second * getReadEnergy(it->first);
  for (auto it = partition_stores.begin(); it != partition_stores.end(); ++it)
    store_energy += it->second * getWriteEnergy(it->first);
  std::vector<std::string> all_partitions;
  getMemoryBlocks(all_partitions);
  for (auto it = all_partitions.begin(); it != all_partitions.end(); ++it)
    leakage_power += getLeakagePower(*it);

  // Load power and store power are computed per cycle, so we have to average
  // the aggregated per cycle power.
  *avg_dynamic = (load_energy + store_energy) * 1000 / (cycleTime * cycles);
  *avg_leak = leakage_power;
  *avg_power = *avg_dynamic + *avg_leak;
}

float Scratchpad::getTotalArea() {
  std::vector<std::string> all_partitions;
  getMemoryBlocks(all_partitions);
  float mem_area = 0;
  for (auto it = all_partitions.begin(); it != all_partitions.end(); ++it)
    mem_area += getArea(*it);
  return mem_area;
}

void Scratchpad::getMemoryBlocks(std::vector<std::string>& names) {
  for (auto base_it = baseToPartitionID.begin();
       base_it != baseToPartitionID.end();
       ++base_it) {
    std::string base_name = base_it->first;
    unsigned base_id = base_it->second;
    names.push_back(base_name);
  }
}
unsigned Scratchpad::getTotalSize() {
  unsigned total_size = 0;
  for(auto it = sizePerPartition.begin();
      it != sizePerPartition.end(); ++it)
    total_size += *it;
  return total_size;
}

void Scratchpad::increment_loads(std::string partition) {
  occupiedBWPerPartition[partition]++;
  partition_loads[partition]++;
}

void Scratchpad::increment_stores(std::string partition) {
  occupiedBWPerPartition[partition]++;
  partition_stores[partition]++;
}

float Scratchpad::getReadEnergy(std::string baseName) {
  unsigned partition_id = findPartitionID(baseName);
  return readEnergyPerPartition.at(partition_id);
}

float Scratchpad::getWriteEnergy(std::string baseName) {
  unsigned partition_id = findPartitionID(baseName);
  return writeEnergyPerPartition.at(partition_id);
}

float Scratchpad::getLeakagePower(std::string baseName) {
  unsigned partition_id = findPartitionID(baseName);
  return leakPowerPerPartition.at(partition_id);
}

float Scratchpad::getArea(std::string baseName) {
  unsigned partition_id = findPartitionID(baseName);
  return areaPerPartition.at(partition_id);
}
