#include "stencil.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void stencil (int *orig, int *sol, int const *filter)
{
#ifdef DMA_MODE
	dmaLoad(&orig[0], 0, N*sizeof(int));
#endif
	int i, j,k1, k2, sidx, didx, fidx;
	int tmp, Si, SI, Di, Ti;
  outer:for (i=0; i<N-2; i++) {
    inner:for (j=0; j<N-2; j++) {
			Si = 0;
			SI = 0;
			tmp = 0;
			fidx = 0;
			SI = i * N;
			sidx = SI + j ;
			didx = sidx;

			Si = filter[fidx] * orig[sidx];
			tmp = tmp + Si;
			sidx ++;
			fidx ++;

			Si = filter[fidx] * orig[sidx];
			tmp = tmp + Si;
			sidx ++;
			fidx ++;

			Si = filter[fidx] * orig[sidx];
			tmp = tmp + Si;
			sidx ++;
			fidx ++;

			sidx += (N ) - 3;

			Si = filter[fidx] * orig[sidx];
			tmp = tmp + Si;
			sidx ++;
			fidx ++;

			Si = filter[fidx] * orig[sidx];
			tmp = tmp + Si;
			sidx ++;
			fidx ++;

			Si = filter[fidx] * orig[sidx];
			tmp = tmp + Si;
			sidx ++;
			fidx ++;

			sidx = sidx + (N ) - 3;

			Si = filter[fidx] * orig[sidx];
			tmp = tmp + Si;
			sidx ++;
			fidx ++;

			Si = filter[fidx] * orig[sidx];
			tmp = tmp + Si;
			sidx ++;
			fidx ++;

			Si = filter[fidx] * orig[sidx];
			tmp = tmp + Si;
			sidx ++;
			fidx ++;
			sol[didx] = tmp;
		}
	}
#ifdef DMA_MODE
  dmaStore(&sol[0], 0, N*sizeof(int));
#endif
}

int main()
{
	int *OrigImg;
	int *Solution;
	int *Filter;
    int i, j, k;
    int max, min;

 	srand(8650341L);
        max = 2147483646;
        min = 0;
  OrigImg = (int *)malloc(sizeof(int) * N * N);
  Solution = (int *)malloc(sizeof(int) * N * N);
  Filter = (int *) malloc(sizeof(int) * 3 * 3);
	for(i=0;i<N ;i++)
	{
		for(j=0;j<N ;j++)
		{
			OrigImg[i * (N ) + j] = (int)(( rand() * 1.0 * ( max-min) / (RAND_MAX)) + min);
			Solution[i * (N ) + j] = 0;
			//printf("Orig: %d ", OrigImg[i*(34+2) +j]);
		}
	}

	for(i=0;i<3;i++)
	{
		for(j=0;j<3;j++)
		{
			Filter[i*3 + j] = (int)(( rand() * 1.0 * (max - min ) / (RAND_MAX)) + min);
			//if (i == 0 && j == 0)
      //  Filter[i*3 + j] = 1;
      //else
      //  Filter[i*3 + j] = 0;
		}
	}

#ifdef GEM5
  resetGem5Stats();
#endif
	stencil(&OrigImg[0], &Solution[0], &Filter[0]);
#ifdef GEM5
  dumpGem5Stats("stencil");
#endif

	for(i=0;i<(4);i++)
		{
			for(j=0;j<4;j++)
			{
				printf("%d, %d, %d,%d\n ", i, j, OrigImg[i*(N ) + j], Solution[i *
        (N) + j]);
			}
		}
	printf("Success!!\n");

	return 0;
}
