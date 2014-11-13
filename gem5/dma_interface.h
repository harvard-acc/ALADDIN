#ifndef __DMA_INTERFACE_H__
#define __DMA_INTERFACE_H__

// size in bits for LLVM-Tracer parsing
int dmaLoad ( int *addr, int size)
{
  asm("");
  return addr+size;
}
int dmaStore ( int *addr, int size)
{
  asm("");
  return addr+size;
}

#endif
