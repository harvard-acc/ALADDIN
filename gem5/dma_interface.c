#include "dma_interface.h"

int dmaLoad ( int *addr, int offset, int size)
{
  asm("");
  return addr[0]+size+offset;
}
int dmaStore ( int *addr, int offset, int size)
{
  asm("");
  return addr[0]+size+offset;
}
