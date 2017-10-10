#include <assert.h>
#include <string.h>
#include "dma_interface.h"

#ifdef DMA_INTERFACE_V2
void* dmaLoad(void* base_addr, size_t src_off, size_t dst_off, size_t size) {
  assert(size > 0);
  memmove(base_addr + dst_off, base_addr + src_off, size);
  return NULL;
}

void* dmaStore(void* base_addr, size_t src_off, size_t dst_off, size_t size) {
  assert(size > 0);
  memmove(base_addr + dst_off, base_addr + src_off, size);
  return NULL;
}
#else
void* dmaLoad(void* addr, size_t offset, size_t size) {
  assert(size > 0);
  asm("");
  return NULL;
}

void* dmaStore(void* addr, size_t offset, size_t size) {
  assert(size > 0);
  asm("");
  return NULL;
}
#endif
void dmaFence() {
  asm("");
}
