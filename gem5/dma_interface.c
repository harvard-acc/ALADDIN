#include "dma_interface.h"

int dmaLoad ( int *addr, int size)
{
  asm("");
  return addr[0]+size;
}
int dmaStore ( int *addr, int size)
{
  asm("");
  return addr[0]+size;
}
