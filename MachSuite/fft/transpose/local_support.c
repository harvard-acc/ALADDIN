#include "fft.h"
#include <string.h>

#ifdef GEM5_HARNESS
#include "gem5/gem5_harness.h"
#endif

int INPUT_SIZE = sizeof(struct bench_args_t);

#define EPSILON ((TYPE)1.0e-6)

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  TYPE* host_work_x = malloc_aligned_memcpy(&args->work_x, sizeof(args->work_x));
  TYPE* host_work_y = malloc_aligned_memcpy(&args->work_y, sizeof(args->work_y));
  TYPE* accel_work_x = malloc_aligned(sizeof(args->work_x));
  TYPE* accel_work_y = malloc_aligned(sizeof(args->work_y));
#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      MACHSUITE_FFT_TRANSPOSE, "host_work_x", host_work_x, sizeof(args->work_x));
  mapArrayToAccelerator(
      MACHSUITE_FFT_TRANSPOSE, "host_work_y", host_work_y, sizeof(args->work_y));
  invokeAcceleratorAndBlock(MACHSUITE_FFT_TRANSPOSE);
#else
  fft1D_512(host_work_x, host_work_y, accel_work_x, accel_work_y);
#endif
  memcpy(&args->work_x, host_work_x, sizeof(args->work_x));
  memcpy(&args->work_y, host_work_y, sizeof(args->work_y));
  free(host_work_x);
  free(host_work_y);
  free(accel_work_x);
  free(accel_work_y);
}

/* Input format:
%% Section 1
TYPE[512]: signal (real part)
%% Section 2
TYPE[512]: signal (complex part)
*/

void input_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,TYPE,_array)(s, data->work_x, 512);

  s = find_section_start(p,2);
  STAC(parse_,TYPE,_array)(s, data->work_y, 512);
}

void data_to_input(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->work_x, 512);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->work_y, 512);
}

/* Output format:
%% Section 1
TYPE[512]: freq (real part)
%% Section 2
TYPE[512]: freq (complex part)
*/

void output_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Load input string
  p = readfile(fd);
 
  s = find_section_start(p,1);
  STAC(parse_,TYPE,_array)(s, data->work_x, 512);

  s = find_section_start(p,2);
  STAC(parse_,TYPE,_array)(s, data->work_y, 512);
}

void data_to_output(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->work_x, 512);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->work_y, 512);
}

int check_data( void *vdata, void *vref ) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  struct bench_args_t *ref = (struct bench_args_t *)vref;
  int has_errors = 0;
  int i;
  double real_diff, img_diff;

  for(i=0; i<512; i++) {
    real_diff = data->work_x[i] - ref->work_x[i];
    img_diff = data->work_y[i] - ref->work_y[i];
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
