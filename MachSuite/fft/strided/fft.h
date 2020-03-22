#include <stdio.h>
#include <stdlib.h>
#include "support.h"

#define FFT_SIZE 1024
#define twoPI 6.28318530717959

void fft(double* host_real,
         double* host_img,
         double* host_real_twid,
         double* host_img_twid,
         double* real,
         double* img,
         double* real_twid,
         double* img_twid);

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
        double real[FFT_SIZE];
        double img[FFT_SIZE];
        double real_twid[FFT_SIZE/2];
        double img_twid[FFT_SIZE/2];
};
