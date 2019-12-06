#ifndef __DMA_INTERFACE_H__
#define __DMA_INTERFACE_H__

#include <stddef.h>

#define PAGE_SIZE 4096

#if defined(DMA_INTERFACE_V3)

// Version 3 of the DMA interface enables memcpy operations from arbitrary
// source and destination addresses.

int dmaLoad(void* dst_addr, void* src_host_addr, size_t size);
int dmaStore(void* dst_host_addr, void* src_addr, size_t size);

// The user can explicitly toggle the state of ready bits, if ready mode is
// enabled. This requires support from DMA v3.
int setReadyBits(void* start_addr, size_t size, unsigned value);

int hostLoad(void* dst_addr, void* src_host_addr, size_t size);
int hostStore(void* dst_host_addr, void* src_addr, size_t size);

#elif defined(DMA_INTERFACE_V2)

#error "DMA interface v2 is deprecated!"

// Version 2 of the DMA interface separates source and destination offsets from
// the base address into different fields, and on the host machine, memory is
// actually copied from source to destination (the memory copy will not show up
// in the trace).

void* dmaLoad(void* base_addr, size_t src_off, size_t dst_off, size_t size);
void* dmaStore(void* base_addr, size_t src_off, size_t dst_off, size_t size);

void* hostLoad(void* base_addr, size_t src_off, size_t dst_off, size_t size);
void* hostStore(void* base_addr, size_t src_off, size_t dst_off, size_t size);

#elif defined(DMA_INTERFACE_V1)

#error "DMA interface v1 is deprecated!"

// Version 1 of the DMA interface is now deprecated and will be removed entirely.

void* dmaLoad(void* addr, size_t offset, size_t size);
void* dmaStore(void* addr, size_t offset, size_t size);

void* hostLoad(void* addr, size_t offset, size_t size);
void* hostStore(void* addr, size_t offset, size_t size);

#endif
void dmaFence();

#endif
