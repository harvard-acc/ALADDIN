#include <stdio.h>
#include <stdlib.h>

#define TYPE int

#define N 2048
#define NUMOFBLOCKS 512

#define ELEMENTSPERBLOCK 4
#define RADIXSIZE 4
#define BUCKETSIZE NUMOFBLOCKS*RADIXSIZE
#define MASK 0x3
//SCAN_BLOCK * SCAN_RADIX = BUCKETSIZE

#define SCAN_BLOCK 16
#define SCAN_RADIX BUCKETSIZE/SCAN_BLOCK

