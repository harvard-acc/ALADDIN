#ifndef __LOGICAL_ARRAY__
#define __LOGICAL_ARRAY__

#include <vector>

#include "Partition.h"
#include "ReadyPartition.h"

/* A logical array class for each individual array. The logical array stores
 * the basic information of each array, like array name or partition type. It
 * also includes the physical partitions of each array.*/

class LogicalArray {
 public:
  LogicalArray(std::string _base_name,
                Addr _base_addr,
                PartitionType _partition_type,
                unsigned _partition_factor,
                unsigned _total_size,
                unsigned _word_size,
                unsigned _num_ports,
                bool _ready_mode);
  ~LogicalArray();
  void step();
  void computePartitionSizes(std::vector<int>& size_per_part);
  /* Return true if any Partition can service. */
  bool canService();
  /* Return true if the partition with index part_index can service. */
  bool canService(unsigned part_index, Addr addr, bool isLoad);

  /* Setters. */
  void setBaseAddress(Addr _base_addr) { base_addr = _base_addr; }
  void setReadEnergy(float _read_energy) { part_read_energy = _read_energy; }
  void setWriteEnergy(float _write_energy) {
    part_write_energy = _write_energy;
  }
  void setLeakagePower(float _leak_power) { part_leak_power = _leak_power; }
  void setArea(float _area) { part_area = _area; }
  /* Accessors. */
  /* Find the data block index for address addr in partition part_index. */
  const std::string& getBaseName() { return base_name; }
  size_t getBlockIndex(unsigned part_index, Addr addr);
  size_t getPartitionIndex(Addr addr);
  unsigned getTotalSize() const { return total_size; }
  unsigned getTotalLoads();
  unsigned getTotalStores();

  float getReadEnergy() { return getTotalLoads() * part_read_energy; }
  float getWriteEnergy() { return getTotalStores() * part_write_energy; }
  float getLeakagePower() { return part_leak_power * num_partitions; }
  float getArea() { return part_area * num_partitions; }
  unsigned getNumPartitions() const { return num_partitions; }
  unsigned getWordSize() const { return word_size; }

  /* Write @len bytes contained in @data into this array at address @addr.
   * It is assumed that data is at least len bytes long.
   */
  void writeData(Addr addr, uint8_t* data, size_t len);

  /* Write @len bytes contained in @data into this array at address @addr.
   * It is assumed that data has been allocated at least len bytes of space.
   */
  void readData(Addr addr, size_t len, uint8_t* data);

  /* Increment loads/stores for each Partition. */
  void increment_loads(unsigned part_index);
  void increment_stores(unsigned part_index);

  void increment_loads(unsigned part_index, unsigned num_accesses);
  void increment_stores(unsigned part_index, unsigned num_accesses);

  /* Increment loads/stores for all the Partitions inside the LogicalArray. */
  void increment_streaming_loads(unsigned streaming_size);
  void increment_streaming_stores(unsigned streaming_size);

  /* Ready bit handling. */
  void setReadyBit(unsigned part_index, Addr addr);
  void resetReadyBit(unsigned part_index, Addr addr);
  void setReadyBitRange(Addr addr, unsigned size);
  void resetReadyBitRange(Addr addr, unsigned size);
  void setReadyBits(unsigned part_index) {
    partitions[part_index]->setAllReadyBits();
  }
  void resetReadyBits(unsigned part_index) {
    partitions[part_index]->resetAllReadyBits();
  }
  void setReadyBits() {
    for ( Partition* part : partitions)
      part->setAllReadyBits();
  }
  void resetReadyBits() {
    /*HACK FOR AES SBOX*/
    if (base_name.compare("sbox") == 0)
      return;
    for ( Partition* part : partitions)
      part->resetAllReadyBits();
  }

  void resetStats() {
    for (auto part : partitions) {
      part->resetStats();
    }
  }

  void dumpStats(std::ostream& outfile);

 protected:

  /* Read or write data to this array.
   *
   * If this is a read, then data is modified; otherwise, this LogicalArray
   * is updated with the contents of data.
   */
  void accessData(Addr addr, uint8_t* data, size_t len, bool is_read);

  /* Array label for the LogicalArray. */
  const std::string base_name;
  /* Base address for the LogicalArray.
   *
   * This is the only attribute of the array that is allowed to change across
   * invocations.
   */
  Addr base_addr;
  /* Partition types: block or cyclic. */
  const PartitionType partition_type;
  /* Num of partitions inside this LogicalArray. */
  const unsigned num_partitions;
  /* Total size in Bytes for all the partitions. */
  const unsigned total_size;
  /* Word size for each partition. */
  const unsigned word_size;
  /* Num of ports for each partition. */
  const unsigned num_ports;
  /* Size of each partition. */
  std::vector<int> size_per_part;
  /* All the Partitions inside the same LogicalArray have the same
   * energy/power/area characteristics. */
  /* Per access read energy for each partition. */
  float part_read_energy;
  /* Per access write  energy for each partition. */
  float part_write_energy;
  /* Leakage power for each partition. */
  float part_leak_power;
  /* Area for each partititon. */
  float part_area;
  /* A list of Partitions that are part of the LogicalArray. */
  std::vector<Partition*> partitions;
};

#endif
