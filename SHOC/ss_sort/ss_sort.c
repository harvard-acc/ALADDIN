#include "ss_sort.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void print(int *a, int size)
{
	int i;

	for (i = 0; i < size; i++)
		printf("%d\t", a[i]);
}

//partition goes to bucket, a, b, sum
typedef enum { false, true } bool;
void local_scan(int bucket[BUCKETSIZE])
{
  int radixID, i;
  //l
  loop1_outer:for (radixID = 0; radixID < SCAN_RADIX; ++radixID)
    //fully unroll
    loop1_inner:for (i = 1; i < SCAN_BLOCK; ++i)
    {
      bucket[radixID * SCAN_BLOCK + i] += bucket[radixID * SCAN_BLOCK + i - 1];
    }
}

void sum_scan(int sum[SCAN_RADIX], int bucket[BUCKETSIZE])
{
  int radixID;
  sum[0] = 0;
  //s
  loop2:for (radixID = 1; radixID < SCAN_RADIX; ++radixID)
    sum[radixID] = sum[radixID -1] + bucket[radixID * SCAN_BLOCK - 1];
}

void last_step_scan(int bucket[BUCKETSIZE], int sum[SCAN_RADIX])
{
  int radixID, i;
  //l
  loop3_outer:for (radixID = 0; radixID < SCAN_RADIX; ++radixID)
    //fully unroll
    loop3_inner:for (i = 0; i < SCAN_BLOCK; ++i)
    {
      bucket[radixID * SCAN_BLOCK + i] =
        bucket[radixID * SCAN_BLOCK + i ] + sum[radixID];
    }

}

void init(int bucket[BUCKETSIZE])
{
  int radixID, i;
  //same as partition factor! p
  loop1_outer:for (radixID = 0; radixID < BUCKETSIZE; ++radixID)
      bucket[radixID] = 0;
}


void hist(int bucket[BUCKETSIZE], int a[N], int exp)
{
  int blockID = 0, maskID = 0;
//h
  loop2:for (blockID = 0; blockID < NUMOFBLOCKS; blockID++)
  {
    loop1: for ( maskID = 0; maskID < 4; maskID++)
      bucket[((a[blockID * ELEMENTSPERBLOCK + maskID] >> exp)  & 0x3) * NUMOFBLOCKS + blockID + 1]++;
  }
}




void update(int b[N], int bucket[BUCKETSIZE], int a[N], int exp)
{
  int blockID = 0, maskID = 0;
  //unroll == h
  loop3:for (blockID = 0; blockID < NUMOFBLOCKS; blockID++)
  {
    loop1: for (maskID = 0; maskID < 4; maskID++) {
      int a_value =  a[blockID * ELEMENTSPERBLOCK + maskID];
      int bucket_value = bucket[((a_value >> exp)  & 0x3) * NUMOFBLOCKS + blockID];
      b[bucket_value] = a_value;
      bucket[((a_value >> exp)  & 0x3) * NUMOFBLOCKS + blockID] = bucket_value +1;
    }
  }
}

void ss_sort(int a[N], int b[N], int bucket[BUCKETSIZE], int sum[SCAN_RADIX]){
	int i, exp = 0;
  bool flag = 0;
#ifdef DMA_MODE
  dmaLoad(&a[0], 0 * 1024 * sizeof(int), PAGE_SIZE);
  dmaLoad(&a[0], 1 * 1024 * sizeof(int), PAGE_SIZE);
#endif
	for (exp = 0; exp < 2; exp+=2){
    //NEW TRY
    //BLOCKING
    //4 keys per block
    //int bucket[BUCKETSIZE];
   //HIST
    init(bucket);
    if (flag == 0)
      hist(bucket, a, exp);
    else
      hist(bucket, b, exp);

    //SCAN
    //ignore for now.
    local_scan(bucket);
    sum_scan(sum, bucket);
    last_step_scan(bucket, sum);

    //update
    if (flag == 0)
    {
      update(b, bucket, a, exp);
      flag = 1;
    }
    else
    {
      update(a, bucket, b, exp);
      flag = 0;
    }
	}
#ifdef DMA_MODE
  dmaStore(&a[0], 0 * 1024 * sizeof(int), PAGE_SIZE);
  dmaStore(&a[0], 1 * 1024 * sizeof(int), PAGE_SIZE);
#endif
}

int main()
{
	int i;
	int *arr;
  int *bucket;
  bucket = (int*) malloc(sizeof(int) * BUCKETSIZE);
  int *b;
  int *sum;
  b = (int*) malloc(sizeof(int) * N);
  arr = (int*)malloc(sizeof(int) * N);
  sum = (int*)malloc(sizeof(int) * SCAN_RADIX);
        float max, min;
	srand(8650341L);
  max = 255;
  min = 0;

        for(i=0; i<N; i++){
                arr[i] = (TYPE)(((double) rand() / (RAND_MAX)) * (max-min) +
                min);
        }

	//print(&arr[0], N);

#ifdef GEM5
  resetGem5Stats();
#endif
	ss_sort(arr, b, bucket, sum);
#ifdef GEM5
  dumpGem5Stats("ss_sort");
#endif

	//printf("\nSORTED : ");
	print(&arr[0], 1);
	//printf("\n");

	return 0;
}
