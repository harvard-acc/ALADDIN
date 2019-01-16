#include <vector>

#include "LogicalArray.h"

LogicalArray::LogicalArray(std::string _base_name,
                           Addr _base_addr,
                           PartitionType _partition_type,
                           unsigned _partition_factor,
                           unsigned _total_size,
                           unsigned _word_size,
                           unsigned _num_ports,
                           bool _ready_mode)
    : base_name(_base_name), base_addr(_base_addr),
      partition_type(_partition_type), num_partitions(_partition_factor),
      total_size(_total_size), word_size(_word_size), num_ports(_num_ports) {

  if (base_addr == 0) {
    std::cerr
        << "[WARNING]: Array \"" << base_name
        << "\" has a base address of zero! This is probably incorrect, but as "
           "long as it is never accessed, it will not cause an error."
        << std::endl;
  }

  /* For now, compute the maximum partition size and use that for power
   * calculations. */
  size_t part_size = ceil(
      ((float)total_size/word_size) / num_partitions) * word_size;

  if (total_size == 0 || part_size == 0) {
    std::cerr << "ERROR: Logical array " << base_name
              << " has invalid size! Total size: " << total_size
              << ", partition size: " << part_size << std::endl;
    assert(false);
  }

  uca_org_t cacti_result = cactiWrapper(part_size, word_size, num_ports);
  // set read/write/leak/area per partition
  // power in mW, energy in pJ, area in mm2
  part_read_energy = cacti_result.power.readOp.dynamic * 1e+12;
  part_write_energy = cacti_result.power.writeOp.dynamic * 1e+12;
  part_leak_power = cacti_result.power.readOp.leakage * 1000;
  part_area = cacti_result.area;

  /* To ensure that we can accurately simulate loaded and stored values, be
   * more precise with the actual partition objects. */
  computePartitionSizes(size_per_part);
  for (auto part_size : size_per_part) {
    if (_ready_mode) {
      ReadyPartition* curr_part = new ReadyPartition();
      curr_part->setSize(part_size, word_size);
      curr_part->setNumPorts(num_ports);
      partitions.push_back(curr_part);
    } else {
      Partition* curr_part = new Partition();
      curr_part->setSize(part_size, word_size);
      curr_part->setNumPorts(num_ports);
      partitions.push_back(curr_part);
    }
  }
  /* HACK for machsuite aes. */
  if (base_name.compare("sbox") == 0) {
    setReadyBits();
  }
}

LogicalArray::~LogicalArray() {
  for ( auto part : partitions)
    delete part;
}

/* Compute the size of each partition. Depending on the word size and array
 * size, not every partition will be the same size.
 */
void LogicalArray::computePartitionSizes(std::vector<int>& size_per_part) {
  size_per_part.resize(num_partitions);
  /* First, compute the minimium size of a partition. */
  int min_size = (total_size / word_size / num_partitions) * word_size;
  int remaining_size = total_size - min_size * num_partitions;
  for (size_t i = 0; i < num_partitions; i++)
    size_per_part[i] = min_size;
  /* If there is any remainder, then we have to split this evenly across as
   * many partitions as possible.
   */
  if (remaining_size > 0) {
    size_t remaining_partitions = remaining_size / word_size;
    assert(remaining_partitions < num_partitions);
    for (size_t i = 0; i < remaining_partitions; i++)
      size_per_part[i] += word_size;
  }
}

size_t LogicalArray::getPartitionIndex(Addr addr) {
  int rel_addr = addr - base_addr;
  if (rel_addr < 0)
    throw ArrayAccessException(base_name + ": Array offset is negative.");
  if (rel_addr >= (int)total_size) {
    std::stringstream error_msg;
    error_msg << base_name << ": Attempting to access offset " << rel_addr
              << ", but total size = " << total_size;
    throw ArrayAccessException(error_msg.str());
  }
  size_t part_index = 0;
  if (partition_type == cyclic) {
    /* cyclic partition. */
    part_index = (rel_addr / word_size ) % num_partitions;
  } else {
    /* block partition. */
    for (size_t i = 0; i < size_per_part.size(); i++) {
      if (rel_addr - size_per_part[i] < 0)
        return i;
      rel_addr -= size_per_part[i];
    }
    // If rel_addr > 0, then we've gone past the bounds of the array.
    assert(rel_addr <= 0);
    part_index = size_per_part.size() - 1;
  }
  assert(part_index < num_partitions);
  return part_index;
}

size_t LogicalArray::getBlockIndex(unsigned part_index, Addr addr) {
  if (partition_type == cyclic) {
    /* cyclic partition. */
    Addr rel_addr = addr - base_addr;
    return rel_addr / word_size / num_partitions;
  } else {
    /* block partition. */
    Addr part_base = base_addr;
    for (size_t i = 0; i < part_index; i++) {
      part_base += size_per_part[i];
    }
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

void LogicalArray::accessData(Addr addr,
                              uint8_t* data,
                              size_t len,
                              bool is_read) {
  if (len == 0)
    throw ArrayAccessException("Data access length must be nonzero");
  if (len % word_size != 0)
    throw ArrayAccessException("Data length is not a multiple of word size");
  uint8_t* ptr = nullptr;
  Addr curr_addr = addr;
  for (size_t i = 0; i < len; i += word_size) {
    ptr = &(data[i]);
    unsigned part_index = getPartitionIndex(curr_addr);
    unsigned blk_index = getBlockIndex(part_index, curr_addr);
    if (is_read)
      partitions[part_index]->readBlock(blk_index, ptr);
    else
      partitions[part_index]->writeBlock(blk_index, ptr);
    curr_addr += word_size;
  }
}

void LogicalArray::writeData(Addr addr, uint8_t* data, size_t len) {
  accessData(addr, data, len, false);
}

void LogicalArray::readData(Addr addr, size_t len, uint8_t* data) {
  accessData(addr, data, len, true);
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
    if (!(curr_addr >= base_addr && curr_addr < base_addr + total_size))
      throw ArrayAccessException("Out of bounds array access");
    unsigned part_index = getPartitionIndex(curr_addr);
    unsigned blk_index = getBlockIndex(part_index, curr_addr);
    partitions[part_index]->setReadyBit(blk_index);
  }
}

void LogicalArray::resetReadyBitRange(Addr addr, unsigned size) {
  for (unsigned curr_size = 0; curr_size < size; curr_size += word_size) {
    Addr curr_addr = addr + curr_size;
    if (!(curr_addr >= base_addr && curr_addr < base_addr + total_size))
      throw ArrayAccessException("Out of bounds array access");
    unsigned part_index = getPartitionIndex(curr_addr);
    unsigned blk_index = getBlockIndex(part_index, curr_addr);
    partitions[part_index]->resetReadyBit(blk_index);
  }
}
