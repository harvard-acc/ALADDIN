#ifndef __PARTITION__
#define __PARTITION__

#include <assert.h>
#include <iostream>
#include <math.h>
#include <unordered_map>
#include <vector>
#include "power_func.h"
#include "generic_func.h"
#include "typedefs.h"

enum PartitionType { block, cyclic };

/* This defines a Partition class for each partitioned scratchpad. */
class Partition {
 public:
  Partition() {
    occupied_bw = 0;
    loads = 0;
    stores = 0;
    size = 0;
    word_size = 4;
    num_words = 0;
    num_ports = 1;
  }
  virtual ~Partition(){};
  /* Setters. */
  virtual void setSize(unsigned _size, unsigned _word_size);
  void setNumPorts(unsigned _num_ports) { num_ports = _num_ports; }
  void setOccupiedBW(unsigned _bw) { occupied_bw = _bw; }
  void resetOccupiedBW() { occupied_bw = 0; }
  /* Getters */
  unsigned getOccupiedBW() { return occupied_bw; }
  unsigned getLoads() { return loads; }
  unsigned getStores() { return stores; }

  /* Return true if there is available bandwidth. */
  bool canService() { return occupied_bw < num_ports; }
  /* Return true if there is available bandwidth. */
  virtual bool canService(unsigned blk_index,
                          bool isLoad){
    return canService();
  }
  virtual void setReadyBit(unsigned blk_index) {}
  virtual void resetReadyBit(unsigned blk_index) {}
  virtual void setAllReadyBits() {}
  virtual void resetAllReadyBits() {}
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

 protected:
  /* Num of bandwidth used in the current cycle. Reset every cycle. */
  unsigned occupied_bw;
  /* Num of ports available. */
  unsigned num_ports;
  /* Total number of loads so far. */
  unsigned loads;
  /* Total number of stores so far. */
  unsigned stores;
  /* Total number of bytes. */
  unsigned size;
  /* Number of bytes per word. */
  unsigned word_size;
  /* Total number of words. */
  unsigned num_words;
};


#endif
