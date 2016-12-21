#ifndef __DMA_INTERFACE_H__
#define __DMA_INTERFACE_H__

#include <stddef.h>

#define PAGE_SIZE 4096

// size in bytes for LLVM-Tracer parsing
void* dmaLoad(void *addr, size_t offset, size_t size);
void* dmaStore(void *addr, size_t offset, size_t size);

#endif
