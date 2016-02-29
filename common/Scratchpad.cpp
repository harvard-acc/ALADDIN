#include "Scratchpad.h"

LogicalArray::LogicalArray(std::string _base_name,
                             uint64_t _base_addr,
                             PartitionType _partition_type,
                             unsigned _partition_factor,
                             unsigned _total_size,
                             unsigned _word_size,
                             unsigned _num_ports) {
  base_name = _base_name;
  base_addr = _base_addr;
  partition_type = _partition_type;
  num_partitions = _partition_factor;
  total_size = _total_size;
  word_size = _word_size;
  num_ports = _num_ports;
  unsigned partition_size = ceil(((float)total_size) / num_partitions);
  uca_org_t cacti_result = cactiWrapper(partition_size, word_size);
  // set read/write/leak/area per partition
  // power in mW, energy in pJ, area in mm2
  part_read_energy = cacti_result.power.readOp.dynamic * 1e+12;
  part_write_energy = cacti_result.power.writeOp.dynamic * 1e+12;
  part_leak_power = cacti_result.power.readOp.leakage * 1000;
  part_area = cacti_result.area;

  for (unsigned i = 0; i < num_partitions; ++i) {
    Partition* curr_part = new Partition();
    curr_part->setSize(partition_size);
    curr_part->setNumPorts(num_ports);
    partitions.push_back(curr_part);
  }
}

LogicalArray::~LogicalArray() {
  for ( Partition* part : partitions)
    delete part;
}

void LogicalArray::step() {
  for ( Partition* part : partitions)
    part->resetOccupiedBW();
}

bool LogicalArray::canService() {
  for ( Partition* part : partitions) {
    if (part->canService())
      return true;
  }
  return false;
}

bool LogicalArray::canService(unsigned part_index) {
  return partitions[part_index]->canService();
}

unsigned LogicalArray::getTotalLoads() {
  unsigned total_counter = 0;
  for ( Partition* part : partitions)
    total_counter += part->getLoads();
  return total_counter;
}

unsigned LogicalArray::getTotalStores() {
  unsigned total_counter = 0;
  for ( Partition* part : partitions)
    total_counter += part->getStores();
  return total_counter;
}

void LogicalArray::increment_loads(unsigned part_index) {
  partitions[part_index]->increment_loads();
}

void LogicalArray::increment_stores(unsigned part_index) {
  partitions[part_index]->increment_stores();
}

void LogicalArray::increment_loads(unsigned part_index,
                                    unsigned num_accesses) {
  partitions[part_index]->increment_loads(num_accesses);
}

void LogicalArray::increment_stores(unsigned part_index,
                                     unsigned num_accesses) {
  partitions[part_index]->increment_stores(num_accesses);
}

void LogicalArray::increment_streaming_loads(unsigned streaming_size) {
  unsigned num_accesses_per_part = streaming_size / num_partitions / word_size;
  for ( Partition* part : partitions ) {
    part->increment_loads(num_accesses_per_part);
  }
}

void LogicalArray::increment_streaming_stores(unsigned streaming_size) {
  unsigned num_accesses_per_part = streaming_size / num_partitions / word_size;
  for ( Partition* part : partitions ) {
    part->increment_stores(num_accesses_per_part);
  }
}

Scratchpad::Scratchpad(unsigned p_ports_per_part, float cycle_time) {
  numOfPortsPerPartition = p_ports_per_part;
  cycleTime = cycle_time;
}

Scratchpad::~Scratchpad() {}

void Scratchpad::clear() {
  for (auto it = logical_arrays.begin(); it != logical_arrays.end(); ++it) {
    delete it->second;
  }
  logical_arrays.clear();
}
// wordsize in bytes
void Scratchpad::setScratchpad(std::string baseName,
                               uint64_t base_addr,
                               PartitionType part_type,
                               unsigned part_factor,
                               unsigned num_of_bytes,
                               unsigned wordsize) {
  assert(!partitionExist(baseName));
  LogicalArray* curr_base = new LogicalArray(
      baseName, base_addr, part_type, part_factor, num_of_bytes, wordsize);
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
                                     unsigned part_index) {
  if (logical_arrays[baseName]->canService(part_index))
    return true;
  else
    return false;
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
  logical_arrays[array_label]->increment_loads(index);
}

void Scratchpad::increment_stores(std::string array_label, unsigned index) {
  logical_arrays[array_label]->increment_stores(index);
}

void Scratchpad::increment_dma_loads(std::string array_label,
                                     unsigned dma_size) {
  logical_arrays[array_label]->increment_streaming_stores(dma_size);
}

void Scratchpad::increment_dma_stores(std::string array_label,
                                      unsigned dma_size) {
  logical_arrays[array_label]->increment_streaming_loads(dma_size);
}
