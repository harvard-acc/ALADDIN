#include "LogicalArray.h"

LogicalArray::LogicalArray(std::string _base_name,
                             Addr _base_addr,
                             PartitionType _partition_type,
                             unsigned _partition_factor,
                             unsigned _total_size,
                             unsigned _word_size,
                             unsigned _num_ports,
                             bool _ready_mode) {
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
    if (_ready_mode) {
      ReadyPartition* curr_part = new ReadyPartition();
      curr_part->setSize(partition_size, word_size);
      curr_part->setNumPorts(num_ports);
      partitions.push_back(curr_part);
    } else {
      Partition* curr_part = new Partition();
      curr_part->setSize(partition_size, word_size);
      curr_part->setNumPorts(num_ports);
      partitions.push_back(curr_part);
    }
  }
}

LogicalArray::~LogicalArray() {
  for ( auto part : partitions)
    delete part;
}

unsigned LogicalArray::getPartitionIndex(Addr addr) {
  Addr rel_addr = addr - base_addr;
  if (partition_type == cyclic) {
    /* cyclic partition. */
    return (rel_addr / word_size ) % num_partitions;
  } else {
    /* block partition. */
    unsigned partition_size = ceil(((float)total_size) / num_partitions);
    return (int)(rel_addr / word_size / partition_size );
  }
}

unsigned LogicalArray::getBlockIndex(unsigned part_index, Addr addr) {
  if (partition_type == cyclic) {
    /* cyclic partition. */
    Addr rel_addr = addr - base_addr;
    return rel_addr / word_size / num_partitions;
  } else {
    /* block partition. */
    unsigned partition_size = ceil(((float)total_size) / num_partitions);
    Addr part_base = base_addr + part_index * partition_size;
    Addr rel_addr = addr - part_base;
    return rel_addr / word_size;
  }
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

bool LogicalArray::canService(unsigned part_index,
                              Addr addr,
                              bool isLoad) {
  unsigned blk_index = getBlockIndex(part_index, addr);
  return partitions[part_index]->canService(blk_index, isLoad);
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


void LogicalArray::setReadyBit(unsigned part_index, Addr addr) {
  unsigned blk_index = getBlockIndex(part_index, addr);
  partitions[part_index]->setReadyBit(blk_index);

}

void LogicalArray::resetReadyBit(unsigned part_index, Addr addr) {
  unsigned blk_index = getBlockIndex(part_index, addr);
  partitions[part_index]->resetReadyBit(blk_index);
}

void LogicalArray::setReadyBitRange(Addr addr, unsigned size) {

  for (unsigned curr_size = 0; curr_size < size; curr_size += word_size) {
    Addr curr_addr = addr + curr_size;
    unsigned part_index = getPartitionIndex(curr_addr);
    unsigned blk_index = getBlockIndex(part_index, curr_addr);
    partitions[part_index]->setReadyBit(blk_index);
  }
}

void LogicalArray::resetReadyBitRange(Addr addr, unsigned size) {

  for (unsigned curr_size = 0; curr_size < size; curr_size += word_size) {
    Addr curr_addr = addr + curr_size;
    unsigned part_index = getPartitionIndex(curr_addr);
    unsigned blk_index = getBlockIndex(part_index, curr_addr);
    partitions[part_index]->resetReadyBit(blk_index);
  }
}
