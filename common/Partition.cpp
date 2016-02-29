#include "Partition.h"

void Partition::setSize(unsigned _size, unsigned _word_size) {
  size = _size;
  word_size = _word_size;
  num_words = size/word_size;
}
