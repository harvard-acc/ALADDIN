#include "Partition.h"

// TODO: There isn't any reason why this can't be done in the constructor, so
// put it there.
void Partition::setSize(unsigned _size, unsigned _word_size) {
  size = _size;
  word_size = _word_size;
  num_words = size/word_size;
  data.resize(num_words);
  for (auto& blk : data)
    blk = new uint8_t[word_size];
}

Partition::~Partition() {
  for (auto& blk : data) {
    if (blk)
      delete[] blk;
  }
}
