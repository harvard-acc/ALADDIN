#include "Scratchpad.h"

Scratchpad::Scratchpad(
    unsigned ports_per_part, float cycle_time, bool _ready_mode) {
  num_ports = ports_per_part;
  cycleTime = cycle_time;
  ready_mode = _ready_mode;
}

Scratchpad::~Scratchpad() { clear(); }

void Scratchpad::clear() {
  for (auto it = logical_arrays.begin(); it != logical_arrays.end(); ++it) {
    delete it->second;
  }
  logical_arrays.clear();
}

// wordsize in bytes
void Scratchpad::setScratchpad(std::string baseName,
                               Addr base_addr,
                               PartitionType part_type,
                               unsigned part_factor,
                               unsigned num_of_bytes,
                               unsigned wordsize) {
  assert(!partitionExist(baseName));
  LogicalArray* curr_base = new LogicalArray(
      baseName, base_addr, part_type, part_factor, num_of_bytes, wordsize,
      num_ports, ready_mode);
  logical_arrays[baseName] = curr_base;
}

void Scratchpad::step() {
  for (auto it = logical_arrays.begin(); it != logical_arrays.end(); ++it)
    it->second->step();
}

bool Scratchpad::partitionExist(std::string baseName) {
  auto partition_it = logical_arrays.find(baseName);
  if (partition_it != logical_arrays.end())
    return true;
  else
    return false;
}

bool Scratchpad::canService() {
  for (auto it = logical_arrays.begin(); it != logical_arrays.end(); ++it) {
    if (it->second->canService())
      return true;
  }
  return false;
}

bool Scratchpad::canServicePartition(std::string baseName,
                                     unsigned part_index,
                                     Addr addr,
                                     bool isLoad) {
  return getLogicalArray(baseName)->canService(part_index, addr, isLoad);
}

// power in mW, energy in pJ, time in ns
void Scratchpad::getAveragePower(unsigned int cycles,
                                 float* avg_power,
                                 float* avg_dynamic,
                                 float* avg_leak) {
  float load_energy = 0;
  float store_energy = 0;
  float leakage_power = 0;
  for (auto it = logical_arrays.begin(); it != logical_arrays.end(); ++it) {
    load_energy += it->second->getReadEnergy();
    store_energy += it->second->getWriteEnergy();
    leakage_power += it->second->getLeakagePower();
  }

  // Load power and store power are computed per cycle, so we have to average
  // the aggregated per cycle power.
  *avg_dynamic = (load_energy + store_energy) / (cycleTime * cycles);
  *avg_leak = leakage_power;
  *avg_power = *avg_dynamic + *avg_leak;
}

float Scratchpad::getTotalArea() {
  float mem_area = 0;
  for (auto it = logical_arrays.begin(); it != logical_arrays.end(); ++it)
    mem_area += it->second->getArea();
  return mem_area;
}

void Scratchpad::getMemoryBlocks(std::vector<std::string>& names) {
  for (auto it = logical_arrays.begin(); it != logical_arrays.end(); ++it) {
    names.push_back(it->first);
  }
}
void Scratchpad::increment_loads(std::string array_label, unsigned index) {
  getLogicalArray(array_label)->increment_loads(index);
}

void Scratchpad::increment_stores(std::string array_label, unsigned index) {
  getLogicalArray(array_label)->increment_stores(index);
}

void Scratchpad::increment_dma_loads(std::string array_label,
                                     unsigned dma_size) {
  getLogicalArray(array_label)->increment_streaming_stores(dma_size);
}

void Scratchpad::increment_dma_stores(std::string array_label,
                                      unsigned dma_size) {
  getLogicalArray(array_label)->increment_streaming_loads(dma_size);
}

void Scratchpad::dumpStats(std::ofstream& outfile) {
  for (auto it = logical_arrays.begin(); it != logical_arrays.end(); it++) {
    const std::string& name = it->first;
    LogicalArray* array = it->second;
    outfile << "Array: " << name << ", size = " << array->getTotalSize()
            << ", partitions = " << array->getNumPartitions()
            << ", word size = " << array->getWordSize()
            << ", reads = " << array->getTotalLoads()
            << ", writes = " << array->getTotalStores() << "\n";
  }
  outfile << "===============\n";
}
