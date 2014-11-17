#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NUM 1024
#define twoPI 6.28318530717959

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
  fft(args->real, args->img, args->real_twid, args->img_twid );
}

////////////////////////////////////////////////////////////////////////////////


