#include "gemm.h"
#include <string.h>

#ifdef GEM5_HARNESS
#include "gem5/gem5_harness.h"
#endif

int INPUT_SIZE = sizeof(struct bench_args_t);

#define EPSILON ((TYPE)1.0e-6)

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  TYPE* host_m1 = malloc_aligned_memcpy(&args->m1, sizeof(args->m1));
  TYPE* host_m2 = malloc_aligned_memcpy(&args->m2, sizeof(args->m2));
  TYPE* host_prod = malloc_aligned_memcpy(&args->prod, sizeof(args->prod));
  TYPE* accel_m1 = malloc_aligned(sizeof(args->m1));
  TYPE* accel_m2 = malloc_aligned(sizeof(args->m2));
  TYPE* accel_prod = calloc_aligned(sizeof(args->prod));
#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      MACHSUITE_GEMM_BLOCKED, "host_m1", host_m1, sizeof(args->m1));
  mapArrayToAccelerator(
      MACHSUITE_GEMM_BLOCKED, "host_m2", host_m2, sizeof(args->m2));
  mapArrayToAccelerator(
      MACHSUITE_GEMM_BLOCKED, "host_prod", host_prod, sizeof(args->prod));
  invokeAcceleratorAndBlock(MACHSUITE_GEMM_BLOCKED);
#else
  bbgemm(host_m1, host_m2, host_prod, accel_m1, accel_m2, accel_prod);
#endif
  memcpy(&args->prod, host_prod, sizeof(args->prod));
  free(host_m1);
  free(host_m2);
  free(host_prod);
  free(accel_m1);
  free(accel_m2);
  free(accel_prod);
}

/* Input format:
%% Section 1
TYPE[N]: matrix 1
%% Section 2
TYPE[N]: matrix 2
*/

void input_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,TYPE,_array)(s, data->m1, N);

  s = find_section_start(p,2);
  STAC(parse_,TYPE,_array)(s, data->m2, N);

}

void data_to_input(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->m1, N);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->m2, N);
}

/* Output format:
%% Section 1
TYPE[N]: output matrix
*/

void output_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,TYPE,_array)(s, data->prod, N);
}

void data_to_output(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->prod, N);
}

int check_data( void *vdata, void *vref ) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  struct bench_args_t *ref = (struct bench_args_t *)vref;
  int has_errors = 0;
  int r,c;
  TYPE diff;

  for( r=0; r<row_size; r++ ) {
    for( c=0; c<col_size; c++ ) {
      diff = data->prod[r*row_size + c] - ref->prod[r*row_size+c];
      has_errors |= (diff<-EPSILON) || (EPSILON<diff);
    }
  }

  // Return true if it's correct.
  return !has_errors;
}
