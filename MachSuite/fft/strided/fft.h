#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NUM 1024
#define twoPI 6.28318530717959

#ifdef GEM5_HARNESS
#include "gem5/aladdin_sys_connection.h"
#include "gem5/aladdin_sys_constants.h"
#endif

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void fft(double real[NUM], double img[NUM], double real_twid[NUM], double img_twid[NUM]);


////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
        double real[NUM];
        double img[NUM];
        double real_twid[NUM];
        double img_twid[NUM];
};
int INPUT_SIZE = sizeof(struct bench_args_t);

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;

#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      MACHSUITE_FFT_STRIDED, "real", (void*)&args->real, sizeof(args->real));
  mapArrayToAccelerator(
      MACHSUITE_FFT_STRIDED, "img", (void*)&args->img, sizeof(args->img));
  mapArrayToAccelerator(
      MACHSUITE_FFT_STRIDED, "real_twid", (void*)&args->real_twid,
                                          sizeof(args->real_twid));
  mapArrayToAccelerator(
      MACHSUITE_FFT_STRIDED, "img_twid", (void*)&args->img_twid,
                                          sizeof(args->img_twid));
  invokeAcceleratorAndBlock(MACHSUITE_FFT_STRIDED);
#else
  fft(args->real, args->img, args->real_twid, args->img_twid );
#endif
}

////////////////////////////////////////////////////////////////////////////////


