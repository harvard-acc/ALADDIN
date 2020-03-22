#include "spmv.h"
#include <string.h>

#ifdef GEM5_HARNESS
#include "gem5/gem5_harness.h"
#endif

int INPUT_SIZE = sizeof(struct bench_args_t);

#define EPSILON 1.0e-6

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  TYPE* host_val = malloc_aligned_memcpy(&args->val, sizeof(args->val));
  int32_t* host_cols = malloc_aligned_memcpy(&args->cols, sizeof(args->cols));
  int32_t* host_rowDelimiters = malloc_aligned_memcpy(&args->rowDelimiters, sizeof(args->rowDelimiters));
  TYPE* host_vec = malloc_aligned_memcpy(&args->vec, sizeof(args->vec));
  TYPE* host_out = malloc_aligned(sizeof(args->out));
  TYPE* accel_val = malloc_aligned(sizeof(args->val));
  int32_t* accel_cols = malloc_aligned(sizeof(args->cols));
  int32_t* accel_rowDelimiters = malloc_aligned(sizeof(args->rowDelimiters));
  TYPE* accel_vec = malloc_aligned(sizeof(args->vec));
  TYPE* accel_out = calloc_aligned(sizeof(args->out));
#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      MACHSUITE_SPMV_CRS, "host_val", host_val, sizeof(args->val));
  mapArrayToAccelerator(
      MACHSUITE_SPMV_CRS, "host_cols", host_cols, sizeof(args->cols));
  mapArrayToAccelerator(
      MACHSUITE_SPMV_CRS, "host_rowDelimiters", host_rowDelimiters,
      sizeof(args->rowDelimiters));
  mapArrayToAccelerator(
      MACHSUITE_SPMV_CRS, "host_vec", host_vec, sizeof(args->vec));
  mapArrayToAccelerator(
      MACHSUITE_SPMV_CRS, "host_out", host_out, sizeof(args->out));
  invokeAcceleratorAndBlock(MACHSUITE_SPMV_CRS);
#else
  spmv(host_val, host_cols, host_rowDelimiters, host_vec, host_out,
       accel_val, accel_cols, accel_rowDelimiters, accel_vec, accel_out);
#endif
  memcpy(&args->out, host_out, sizeof(args->out));
  free(host_val);
  free(host_cols);
  free(host_rowDelimiters);
  free(host_vec);
  free(host_out);
  free(accel_val);
  free(accel_cols);
  free(accel_rowDelimiters);
  free(accel_vec);
  free(accel_out);
}

/* Input format:
%% Section 1
TYPE[NNZ]: the nonzeros of the matrix
%% Section 2
int32_t[NNZ]: the column index of the nonzeros
%% Section 3
int32_t[N+1]: the start of each row of nonzeros
%% Section 4
TYPE[N]: the dense vector
*/

void input_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,TYPE,_array)(s, data->val, NNZ);

  s = find_section_start(p,2);
  parse_int32_t_array(s, data->cols, NNZ);

  s = find_section_start(p,3);
  parse_int32_t_array(s, data->rowDelimiters, N+1);

  s = find_section_start(p,4);
  STAC(parse_,TYPE,_array)(s, data->vec, N);
}

void data_to_input(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->val, NNZ);

  write_section_header(fd);
  write_int32_t_array(fd, data->cols, NNZ);

  write_section_header(fd);
  write_int32_t_array(fd, data->rowDelimiters, N+1);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->vec, N);
}

/* Output format:
%% Section 1
TYPE[N]: The output vector
*/

void output_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,TYPE,_array)(s, data->out, N);
}

void data_to_output(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->out, N);
}

int check_data( void *vdata, void *vref ) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  struct bench_args_t *ref = (struct bench_args_t *)vref;
  int has_errors = 0;
  int i;
  TYPE diff;

  for(i=0; i<N; i++) {
    diff = data->out[i] - ref->out[i];
    has_errors |= (diff<-EPSILON) || (EPSILON<diff);
  }

  // Return true if it's correct.
  return !has_errors;
}
