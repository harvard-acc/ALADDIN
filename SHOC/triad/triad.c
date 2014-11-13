#include "triad.h"
#include "gem5/dma_interface.h"

void triad(int *a,int *b, int *c, int s){
#ifdef DMA_MODE
	dmaLoad(&a[0], NUM*4*8);
  dmaLoad(&b[0], NUM*4*8);
#endif
  int i;
	for(i=0;i<NUM;i++)
    c[i] = a[i] + s*b[i];
#ifdef DMA_MODE
	dmaStore(&c[0], NUM*4*8);
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

	triad(&a[0],&b[0],&c[0],3);
  FILE *output;
  output = fopen("output.data", "w");
	for(i=0; i<NUM; i++)
    fprintf(output, "%d,", c[i]);
  fprintf(output, "\n");
  fclose(output);
	return a[0];
}
