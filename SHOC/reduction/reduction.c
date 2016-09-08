#include "reduction.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

int reduction(int *in)
{
  int i = 0; int sum = 0;
#ifdef DMA_MODE
	dmaLoad(&in[0], 0, NUM*sizeof(int));
#endif
  sum:for (i = 0; i < NUM; i++)
    sum += in[i];

  return sum;
}


int main()
{
  int *in;
  in = (int *) malloc (sizeof(int) * NUM );

  int i;
  int max = 2147483646;
  int min = 0;
  srand(8650341L);
  for (i = 0; i < NUM; i++)
  {
    in[i] = (int)(min + rand() * 1.0 * (max - min) / (RAND_MAX ));
  }

#ifdef GEM5
  resetGem5Stats();
#endif
  int sum = reduction(&in[0]);
#ifdef GEM5
  dumpGem5Stats("reduction");
#endif
  printf("sum: %d\n", sum);
  return 0;
}
