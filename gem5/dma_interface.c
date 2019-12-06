#include <assert.h>
#include <string.h>
#include "dma_interface.h"

// All _dmaImplN functions must be always inlined or we'll get extra functions
// in the trace.

#if defined(DMA_INTERFACE_V3)

// Starting with version 3, all versioning will be distinguished by the return
// value of the DMA functions.

__attribute__((__always_inline__))
int _dmaImpl3(void* dst_addr, void* src_addr, size_t size) {
  assert(size > 0);
  memmove(dst_addr, src_addr, size);
  return 3;
}

int dmaLoad(void* dst_addr, void* src_host_addr, size_t size) {
  return _dmaImpl3(dst_addr, src_host_addr, size);
}

int dmaStore(void* dst_host_addr, void* src_addr, size_t size) {
  return _dmaImpl3(dst_host_addr, src_addr, size);
}

int setReadyBits(void* start_addr, size_t size, unsigned value) {
  asm("");
  return 0;
}

int hostLoad(void* dst_addr, void* src_host_addr, size_t size) {
  return _dmaImpl3(dst_addr, src_host_addr, size);
}

int hostStore(void* dst_host_addr, void* src_addr, size_t size) {
  return _dmaImpl3(dst_host_addr, src_addr, size);
}

#elif defined(DMA_INTERFACE_V2)

// With version 2 and earlier, we return (void*)NULL and use the number of
// function arguments to distinguish the DMA functions.

__attribute__((__always_inline__))
void* _dmaImpl2(void* base_addr, size_t src_off, size_t dst_off, size_t size) {
  assert(size > 0);
  memmove(base_addr + dst_off, base_addr + src_off, size);
  return NULL;
}

void* dmaLoad(void* base_addr, size_t src_off, size_t dst_off, size_t size) {
  return _dmaImpl2(base_addr, src_off, dst_off, size);
}

void* dmaStore(void* base_addr, size_t src_off, size_t dst_off, size_t size) {
  return _dmaImpl2(base_addr, src_off, dst_off, size);
}

#elif defined(DMA_INTERFACE_V1)

__attribute__((__always_inline__))
void* _dmaImpl1(void* base_addr, size_t offset, size_t size) {
  assert(size > 0);
  asm("");
  return NULL;
}

void* dmaLoad(void* addr, size_t offset, size_t size) {
  return _dmaImpl1(addr, offset, size);
}

void* dmaStore(void* addr, size_t offset, size_t size) {
  return _dmaImpl1(addr, offset, size);
}

#endif

void dmaFence() {
  asm("");
}
