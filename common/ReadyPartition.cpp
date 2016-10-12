#include "ReadyPartition.h"
ReadyPartition::ReadyPartition() : Partition() {}

ReadyPartition::~ReadyPartition() {}

void ReadyPartition::setSize(unsigned _size, unsigned _word_size) {
  Partition::setSize(_size, _word_size);
  ready_bits.resize(num_words);
}
