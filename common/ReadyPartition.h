#ifndef __READY_PARTITION__
#define __READY_PARTITION__

#include "Partition.h"

class ReadyPartition : public Partition {
 public:
  ReadyPartition();
  ~ReadyPartition();

  virtual void writeBlock(unsigned blk_index, uint8_t* data) {
    Partition::writeBlock(blk_index, data);
    setReadyBit(blk_index);
  }

  virtual void readBlock(unsigned blk_index, uint8_t* data) {
    Partition::readBlock(blk_index, data);
  }

  /* Setters. */
  virtual void setSize(unsigned _size, unsigned _word_size);

  /* Set the ready bit for the specific blk_index. */
  virtual void setReadyBit(unsigned blk_index) {
    // Related to bugs ALADDIN-60 and ALADDIN-61.
    assert(blk_index < num_words);
    ready_bits[blk_index] = true;
  }

  /* Reset the ready bit for the specific blk_index. */
  virtual void resetReadyBit(unsigned blk_index) {
    assert(blk_index < num_words);
    ready_bits[blk_index] = false;
  }

  /* Set all the ready bits in the partition. */
  virtual void setAllReadyBits() {
    for (unsigned i = 0; i < num_words; ++i)
      setReadyBit(i);
  }

  /* Reset all the ready bits in the partition. */
  virtual void resetAllReadyBits() {
    for (unsigned i = 0; i < num_words; ++i)
      resetReadyBit(i);
  }

  /* Return true if the data at blk_index is ready and the partition can service. */
  virtual bool canService(unsigned blk_index, bool isLoad) {
    if (Partition::canService()) {
      if (!isLoad) {
        // We do not check ready bit for store for now.
        // We mark data is ready after the store.
        setReadyBit(blk_index);
        return true;
      }
      return ready_bits[blk_index];
    }
    return false;
  }

 protected:
  // Ready bits are stored at word granularity.
  std::vector<bool> ready_bits;
};

#endif
