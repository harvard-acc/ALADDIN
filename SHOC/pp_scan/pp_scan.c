#include "pp_scan.h"

void print(int *a, int size)
{
	int i;

	for (i = 0; i < size; i++)
		printf("%u\t", a[i]);
}

typedef enum { false, true } bool;
void local_scan(int bucket[BUCKETSIZE])
{
  int radixID, i;
  loop1_outter:for (radixID = 0; radixID < SCAN_RADIX; ++radixID)
    loop1_inner:for (i = 1; i < SCAN_BLOCK; ++i)
    {
      bucket[radixID * SCAN_BLOCK + i] += bucket[radixID * SCAN_BLOCK + i - 1];
    }
}

void sum_scan(int sum[SCAN_RADIX], int bucket[BUCKETSIZE])
{
  int radixID;
  sum[0] = 0;
  loop2:for (radixID = 1; radixID < SCAN_RADIX; ++radixID)
    sum[radixID] = sum[radixID -1] + bucket[radixID * SCAN_BLOCK - 1];
}

void last_step_scan(int bucket[BUCKETSIZE], int bucket2[BUCKETSIZE], int sum[SCAN_RADIX])
{
  int radixID, i;
  loop3_outter:for (radixID = 0; radixID < SCAN_RADIX; ++radixID)
    loop3_inner:for (i = 0; i < SCAN_BLOCK; ++i)
    {
      bucket2[radixID * SCAN_BLOCK + i] = 
        bucket[radixID * SCAN_BLOCK + i ] + sum[radixID];
    }

}
void pp_scan(int bucket[BUCKETSIZE], int bucket2[BUCKETSIZE], 
  int sum[SCAN_RADIX])
{
  local_scan(bucket);
  sum_scan(sum, bucket);
  last_step_scan(bucket, bucket2, sum);
}

int main()
{
	int i;
  int  *bucket;
  int  *bucket2;
  bucket = (int*) malloc(sizeof(int) * N);
  bucket2 = (int*) malloc(sizeof(int) * N);
  int sum[SCAN_RADIX];
  int max, min;
	srand(8650341L);
  max = 2048;
  min = 0;	
  for(i=0; i<N; i++){ 
    bucket[i] = (TYPE)(((double) rand() / (RAND_MAX)) * (max-min) + min);
    bucket2[i] = (TYPE)(((double) rand() / (RAND_MAX)) * (max-min) + min);
  }

	print(&bucket[0], 1);
	printf("\n");

	pp_scan(bucket, bucket2, sum);

	print(&bucket[0], 2);
	printf("\n");

	return 0;
}
