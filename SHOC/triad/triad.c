#include "triad.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void triad(int *a,int *b, int *c, int s){
#ifdef DMA_MODE
	dmaLoad(&a[0], 0*1024*sizeof(int), PAGE_SIZE);
	dmaLoad(&a[0], 1*1024*sizeof(int), PAGE_SIZE);
  dmaLoad(&b[0], 0*1024*sizeof(int), PAGE_SIZE);
  dmaLoad(&b[0], 1*1024*sizeof(int), PAGE_SIZE);
#endif
  int i;
  triad:for(i=0;i<NUM;i++)
    c[i] = a[i] + s*b[i];
#ifdef DMA_MODE
  dmaStore(&c[0], 0*1024*sizeof(int), PAGE_SIZE);
  dmaStore(&c[0], 1*1024*sizeof(int), PAGE_SIZE);
#endif
}

int main(){
	int *a, *b, *c;
    a = (int *) malloc (sizeof(int) * NUM);
    b = (int *) malloc (sizeof(int) * NUM);
    c = (int *) malloc (sizeof(int) * NUM);
	int i;
  srand(time(NULL));
	for(i=0; i<NUM; i++){
		c[i] = 0;
		a[i] = rand();
		b[i] = rand();
	}
#ifdef GEM5
  resetGem5Stats();
#endif
	triad(&a[0],&b[0],&c[0],3);
#ifdef GEM5
  dumpGem5Stats("triad");
#endif
  FILE *output;
  output = fopen("output.data", "w");
	for(i=0; i<NUM; i++)
    fprintf(output, "%d,", c[i]);
  fprintf(output, "\n");
  fclose(output);
	return 0;
}
