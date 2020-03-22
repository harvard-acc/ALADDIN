#include "fft.h"
#include <string.h>

#ifdef GEM5_HARNESS
#include "gem5/gem5_harness.h"
#endif

int INPUT_SIZE = sizeof(struct bench_args_t);

#define EPSILON ((double)1.0e-6)

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  double* host_real = malloc_aligned_memcpy(&args->real, sizeof(args->real));
  double* host_img = malloc_aligned_memcpy(&args->img, sizeof(args->img));
  double* host_real_twid = malloc_aligned_memcpy(&args->real_twid, sizeof(args->real_twid));
  double* host_img_twid = malloc_aligned_memcpy(&args->img_twid, sizeof(args->img_twid));
  double* accel_real = malloc_aligned(sizeof(args->real));
  double* accel_img = malloc_aligned(sizeof(args->img));
  double* accel_real_twid = malloc_aligned(sizeof(args->real_twid));
  double* accel_img_twid = malloc_aligned(sizeof(args->img_twid));
#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      MACHSUITE_FFT_STRIDED, "host_real", host_real, sizeof(args->real));
  mapArrayToAccelerator(
      MACHSUITE_FFT_STRIDED, "host_img", host_img, sizeof(args->img));
  mapArrayToAccelerator(
      MACHSUITE_FFT_STRIDED, "host_real_twid", host_real_twid, sizeof(args->real_twid));
  mapArrayToAccelerator(
      MACHSUITE_FFT_STRIDED, "host_img_twid", host_img_twid, sizeof(args->img_twid));
  invokeAcceleratorAndBlock(MACHSUITE_FFT_STRIDED);
#else
  fft(host_real, host_img, host_real_twid, host_img_twid,
      accel_real, accel_img, accel_real_twid, accel_img_twid);
#endif
  memcpy(&args->real, host_real, sizeof(args->real));
  memcpy(&args->img, host_img, sizeof(args->img));
  free(host_real);
  free(host_real_twid);
  free(host_img);
  free(host_img_twid);
  free(accel_real);
  free(accel_real_twid);
  free(accel_img);
  free(accel_img_twid);
}

/* Input format:
%% Section 1
double: signal (real part)
%% Section 2
double: signal (complex part)
%% Section 3
double: twiddle factor (real part)
%% Section 4
double: twiddle factor (complex part)
*/

void input_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_double_array(s, data->real, FFT_SIZE);

  s = find_section_start(p,2);
  parse_double_array(s, data->img, FFT_SIZE);

  s = find_section_start(p,3);
  parse_double_array(s, data->real_twid, FFT_SIZE/2);

  s = find_section_start(p,4);
  parse_double_array(s, data->img_twid, FFT_SIZE/2);
}

void data_to_input(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  write_double_array(fd, data->real, FFT_SIZE);

  write_section_header(fd);
  write_double_array(fd, data->img, FFT_SIZE);

  write_section_header(fd);
  write_double_array(fd, data->real_twid, FFT_SIZE/2);

  write_section_header(fd);
  write_double_array(fd, data->img_twid, FFT_SIZE/2);
}

/* Output format:
%% Section 1
double: freq (real part)
%% Section 2
double: freq (complex part)
*/

void output_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_double_array(s, data->real, FFT_SIZE);

  s = find_section_start(p,2);
  parse_double_array(s, data->img, FFT_SIZE);
}

void data_to_output(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  write_double_array(fd, data->real, FFT_SIZE);

  write_section_header(fd);
  write_double_array(fd, data->img, FFT_SIZE);
}

int check_data( void *vdata, void *vref ) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  struct bench_args_t *ref = (struct bench_args_t *)vref;
  int has_errors = 0;
  int i;
  double real_diff, img_diff;

  for(i=0; i<FFT_SIZE; i++) {
    real_diff = data->real[i] - ref->real[i];
    img_diff = data->img[i] - ref->img[i];
    has_errors |= (real_diff<-EPSILON) || (EPSILON<real_diff);
    //if( has_errors )
      //printf("%d (real): %f (%f)\n", i, real_diff, EPSILON);
    has_errors |= (img_diff<-EPSILON) || (EPSILON<img_diff);
    //if( has_errors )
      //printf("%d (img): %f (%f)\n", i, img_diff, EPSILON);
  }

  // Return true if it's correct.
  return !has_errors;
}
