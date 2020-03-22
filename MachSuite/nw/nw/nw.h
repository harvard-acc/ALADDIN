#include <stdio.h>
#include <stdlib.h>
#include "support.h"

#define ALEN 128
#define BLEN 128

void needwun(char* host_SEQA,
             char* host_SEQB,
             char* host_alignedA,
             char* host_alignedB,
             char* SEQA,
             char* SEQB,
             char* alignedA,
             char* alignedB,
             int* M,
             char* ptr);
////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
  char seqA[ALEN];
  char seqB[BLEN];
  char alignedA[ALEN+BLEN];
  char alignedB[ALEN+BLEN];
  int M[(ALEN+1)*(BLEN+1)];
  char ptr[(ALEN+1)*(BLEN+1)];
};
