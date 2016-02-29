#ifndef __READY_PARTITION__
#define __READY_PARTITION__

#include "Partition.h"

/* A scratchpad block contains data the block is storing and the corresponding
 * status bits */
struct SpadBlock {
  // The data in the block. We are not using the value of the data for now, but
  // we will need this, especially for dmaStore, for end-to-end simulation.
  /*uint8_t *data;*/
  bool ready;

  SpadBlock() {
    /*data = nullptr;*/
    /* By default, the ready bit is set for spad only simulation. */
    ready = true;
  }
};

class ReadyPartition : public Partition {
 public:
  ReadyPartition();
  ~ReadyPartition();
  /* Setters. */
  virtual void setSize(unsigned _size, unsigned _word_size);
  /* Set the ready bit for the specific blk_index. */
  virtual void setReadyBit(unsigned blk_index) {
    dataBlocks[blk_index].ready = true;
  }
  /* Reset the ready bit for the specific blk_index. */
  virtual void resetReadyBit(unsigned blk_index) {
    dataBlocks[blk_index].ready = false;
  }
  /* Set all the ready bits in the partition. */
  virtual void setAllReadyBits() {
    for ( int i = 0; i < num_words; ++i)
      setReadyBit(i);
  }
  /* Reset all the ready bits in the partition. */
  virtual void resetAllReadyBits() {
    for ( int i = 0; i < num_words; ++i)
      resetReadyBit(i);
  }
  /* Return true if the data at blk_index is ready and the partition can service. */
  virtual bool canService(unsigned blk_index, bool isLoad) {
    if (Partition::canService()) {
      if (!isLoad)
        // We do not consider ready bit for store for now.
        return true;
      return dataBlocks[blk_index].ready;
    }
    return false;
  }
 private:
  SpadBlock *dataBlocks;

};

#endif
