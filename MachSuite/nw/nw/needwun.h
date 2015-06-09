/*
Copyright (c) 2014, the President and Fellows of Harvard College.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of Harvard University nor the names of its contributors may
  be used to endorse or promote products derived from this software without
  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>

#define TYPE char

#define N 128
#define M 128
#define N1 (N+1)
#define M1 (M+1)
#define sum_size (M+N)
#define dyn_size  (M+1) * (N+1)

#ifdef GEM5_HARNESS
#include "gem5/aladdin_sys_connection.h"
#include "gem5/aladdin_sys_constants.h"
#endif

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif


/*void needwun(char seqA[N], char seqB[M], char alignedA[N+M], char alignedB[M+N]);*/

void needwun(char SEQA[N], char SEQB[M], char alignedA[sum_size], char alignedB[sum_size],
             int A[dyn_size], char ptr[dyn_size]);
////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
  char seqA[N];
  char seqB[N];
  char alignedA[N+M];
  char alignedB[N+M];
  int A[dyn_size];
  char ptr[dyn_size];
};
int INPUT_SIZE = sizeof(struct bench_args_t);

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;

#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      MACHSUITE_NW_NW, "seqA", (void*)&args->seqA, sizeof(args->seqA));
  mapArrayToAccelerator(
      MACHSUITE_NW_NW, "seqB", (void*)&args->seqB, sizeof(args->seqB));
  mapArrayToAccelerator(
      MACHSUITE_NW_NW, "alignedA", (void*)&args->alignedA,
      sizeof(args->alignedA));
  mapArrayToAccelerator(
      MACHSUITE_NW_NW, "alignedB", (void*)&args->alignedB,
      sizeof(args->alignedB));
  mapArrayToAccelerator(
      MACHSUITE_NW_NW, "A", (void*)&args->A, sizeof(args->A));
  mapArrayToAccelerator(
      MACHSUITE_NW_NW, "ptr", (void*)&args->ptr, sizeof(args->ptr));
  invokeAcceleratorAndBlock(MACHSUITE_NW_NW);
#else
  needwun( args->seqA, args->seqB, args->alignedA, args->alignedB, args->A, args->ptr);
#endif
}

////////////////////////////////////////////////////////////////////////////////
