#ifndef __MEMORY_TYPE_H__
#define __MEMORY_TYPE_H__

// Defines how an array is mapped to the accelerator and how it can be accessed
// via the host.
typedef enum _MemoryType { spad, reg, dma, acp, cache } MemoryType;

#endif
