#ifndef __SCRATCHPAD__
#define __SCRATCHPAD__

#include <assert.h>
#include <iostream>
#include <math.h>
#include <unordered_map>
#include <vector>
#include "power_func.h"
#include "generic_func.h"

enum PartitionType { block, cyclic };

/* Definitions of three classes for Scratchpad processing.
 *
 *   Scratchpad     : Top level class for the entire scratchpad system. It can
 *                  : contain multiple LogicalArrays if the program accesses
 *                  : multiple arrays.
 *
 *   LogicalArray   : Defines the base information of an array, which can be
 *                  : further partitioned into multiple Partitions.
 *
 *   Partition      : Represents each partitioned of a scratchpad.
*/

/* This defines a Partition class for each partitioned scratchpad. */
class Partition {
 public:
  Partition() {
    occupied_bw = 0;
    loads = 0;
    stores = 0;
    size = 0;
    num_ports = 1;
  }
  /* Setters. */
  void setSize(unsigned _size) { size = _size; }
  void setNumPorts(unsigned _num_ports) { num_ports = _num_ports; }
  void setOccupiedBW(unsigned _bw) { occupied_bw = _bw; }
  void resetOccupiedBW() { occupied_bw = 0; }
  /* Getters */
  unsigned getOccupiedBW() { return occupied_bw; }
  unsigned getLoads() { return loads; }
  unsigned getStores() { return stores; }

  /* Return true if there is available bandwidth. */
  bool canService() { return occupied_bw < num_ports; }
  /* Increment the load counter by one and also update the occupied_bw counter.
   */
  void increment_loads() {
    loads++;
    occupied_bw++;
  }
  /* Increment the store counter by one and also update the occupied_bw counter.
   */
  void increment_stores() {
    stores++;
    occupied_bw++;
  }

  /* Increment the laod counter by num_accesses. */
  void increment_loads(unsigned num_accesses) { loads += num_accesses; }
  /* Increment the store counter by num_accesses. */
  void increment_stores(unsigned num_accesses) { stores += num_accesses; }

 private:
  /* Num of bandwidth used in the current cycle. Reset every cycle. */
  unsigned occupied_bw;
  /* Num of ports available. */
  unsigned num_ports;
  /* Total number of loads so far. */
  unsigned loads;
  /* Total number of stores so far. */
  unsigned stores;
  /* Number of bytes. */
  unsigned size;
};

/* A logical array class for each individual array. The logical array stores
 * the basic information of each array, like array name or partition type. It
 * also includes the physical partitions of each array.*/

class LogicalArray {
 public:
  LogicalArray(std::string _base_name,
                uint64_t _base_addr,
                PartitionType _partition_type,
                unsigned _partition_factor,
                unsigned _total_size,
                unsigned _word_size,
                unsigned _num_ports = 1);
  ~LogicalArray();
  std::string getBaseName() { return base_name; }
  void step();
  /* Return true if any Partition can service. */
  bool canService();
  /* Return true if the partition with index part_index can service. */
  bool canService(unsigned part_index);

  /* Setters. */
  void setWordSize(unsigned _word_size) { word_size = _word_size; }
  void setReadEnergy(float _read_energy) { part_read_energy = _read_energy; }
  void setWriteEnergy(float _write_energy) {
    part_write_energy = _write_energy;
  }
  void setLeakagePower(float _leak_power) { part_leak_power = _leak_power; }
  void setArea(float _area) { part_area = _area; }

  /* Accessors. */
  unsigned getTotalLoads();
  unsigned getTotalStores();

  float getReadEnergy() { return getTotalLoads() * part_read_energy; }
  float getWriteEnergy() { return getTotalStores() * part_write_energy; }
  float getLeakagePower() { return part_leak_power * num_partitions; }
  float getArea() { return part_area * num_partitions; }

  /* Increment loads/stores for each Partition. */
  void increment_loads(unsigned part_index);
  void increment_stores(unsigned part_index);

  void increment_loads(unsigned part_index, unsigned num_accesses);
  void increment_stores(unsigned part_index, unsigned num_accesses);

  /* Increment loads/stores for all the Partitions inside the LogicalArray. */
  void increment_streaming_loads(unsigned streaming_size);
  void increment_streaming_stores(unsigned streaming_size);

 private:
  /* A list of Partitions that are part of the LogicalArray. */
  std::vector<Partition*> partitions;
  /* Array label for the LogicalArray. */
  std::string base_name;
  /* Base address for the LogicalArray. */
  uint64_t base_addr;
  PartitionType partition_type;
  /* Num of partitions inside this LogicalArray. */
  unsigned num_partitions;
  /* Total size in Bytes for all the partitions. */
  unsigned total_size;
  /* Word size for each partition. */
  unsigned word_size;
  /* Num of ports for each partition. */
  unsigned num_ports;
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
};

class Scratchpad {
 public:
  Scratchpad(unsigned p_ports_per_part, float cycle_time);
  virtual ~Scratchpad();
  void clear();
  void step();
  void setScratchpad(std::string baseName,
                     uint64_t base_addr,
                     PartitionType part_type,
                     unsigned part_factor,
                     unsigned num_of_bytes,
                     unsigned wordsize);
  bool canService();
  bool canServicePartition(std::string baseName, unsigned index);
  bool partitionExist(std::string baseName);

  /* Increment the loads counter for the specified partition. */
  void increment_loads(std::string array_label, unsigned index);
  /* Increment the stores counter for the specified partition. */
  void increment_stores(std::string array_label, unsigned index);

  /* For accesses that are caused by DMA operations, increment the loads
     counter by num_accesses for the specified partition. */
  void increment_dma_loads(std::string base_part, unsigned num_accesses);
  /* For accesses that are caused by DMA operations, increment the stores
     counter by num_accesses for the specified partition. */
  void increment_dma_stores(std::string base_part, unsigned num_accesses);

  void getMemoryBlocks(std::vector<std::string>& names);

  void getAveragePower(unsigned int cycles,
                       float* avg_power,
                       float* avg_dynamic,
                       float* avg_leakage);
  float getTotalArea();

 private:
  unsigned numOfPortsPerPartition;
  float cycleTime;  // in ns
  std::unordered_map<std::string, LogicalArray*> logical_arrays;
};

#endif
