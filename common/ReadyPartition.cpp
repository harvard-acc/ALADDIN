#include "ReadyPartition.h"
ReadyPartition::ReadyPartition() : Partition() {}

ReadyPartition::~ReadyPartition() {
  delete [] dataBlocks;
}

void ReadyPartition::setSize(unsigned _size, unsigned _word_size) {
  Partition::setSize(_size, _word_size);
  dataBlocks = new SpadBlock[num_words];
}
