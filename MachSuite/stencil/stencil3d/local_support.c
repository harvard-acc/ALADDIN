#include "stencil.h"
#include <string.h>

#ifdef GEM5_HARNESS
#include "gem5/gem5_harness.h"
#endif

int INPUT_SIZE = sizeof(struct bench_args_t);

#define EPSILON (1.0e-6)

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  TYPE* host_orig = malloc_aligned_memcpy(&args->orig, sizeof(args->orig));
  TYPE* host_sol = malloc_aligned(sizeof(args->orig));
  // No host_C - too small, let's just put it into registers.
  TYPE* accel_C = malloc_aligned_memcpy(&args->C, sizeof(args->C));
  TYPE* accel_orig = malloc_aligned(sizeof(args->orig));
  TYPE* accel_sol = malloc_aligned(sizeof(args->orig));
#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      MACHSUITE_STENCIL_3D, "host_orig", host_orig, sizeof(args->orig));
  mapArrayToAccelerator(
      MACHSUITE_STENCIL_3D, "host_sol", host_sol, sizeof(args->sol));
  invokeAcceleratorAndBlock(MACHSUITE_STENCIL_3D);
#else
  stencil3d(host_orig, host_sol, accel_C, accel_orig, accel_sol);
#endif
  memcpy(&args->sol, host_sol, sizeof(args->sol));
  free(host_orig);
  free(host_sol);
  free(accel_C);
  free(accel_orig);
  free(accel_sol);
}

/* Input format:
%% Section 1
TYPE[2]: stencil coefficients (inner/outer)
%% Section 2
TYPE[SIZE]: input matrix
*/

void input_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,TYPE,_array)(s, data->C, 2);

  s = find_section_start(p,2);
  STAC(parse_,TYPE,_array)(s, data->orig, SIZE);
}

void data_to_input(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->C, 2);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->orig, SIZE);
}

/* Output format:
%% Section 1
TYPE[SIZE]: solution matrix
*/

void output_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,TYPE,_array)(s, data->sol, SIZE);
}

void data_to_output(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->sol, SIZE);
}

int check_data( void *vdata, void *vref ) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  struct bench_args_t *ref = (struct bench_args_t *)vref;
  int has_errors = 0;
  int i;
  TYPE diff;

  for(i=0; i<SIZE; i++) {
    diff = data->sol[i] - ref->sol[i];
    has_errors |= (diff<-EPSILON) || (EPSILON<diff);
  }

  // Return true if it's correct.
  return !has_errors;
}
