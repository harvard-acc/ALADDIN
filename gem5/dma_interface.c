#include "dma_interface.h"

void* dmaLoad(void* addr, size_t offset, size_t size) {
  asm("");
  return addr + size + offset;
}

void* dmaStore(void* addr, size_t offset, size_t size) {
  asm("");
  return addr + size + offset;
}
