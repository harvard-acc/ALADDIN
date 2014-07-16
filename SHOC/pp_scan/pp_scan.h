#include <stdio.h>
#include <stdlib.h>

#define TYPE int

#define N 2048
#define NUMOFBLOCKS 512
//#define N 32
//#define NUMOFBLOCKS 8
#define RADIXSIZE 16

#define SCAN_BLOCK 16
#define SCAN_RADIX N/SCAN_BLOCK
#define BUCKETSIZE SCAN_BLOCK*SCAN_RADIX
