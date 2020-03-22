#include "kmp.h"
#include <string.h>

#ifdef GEM5_HARNESS
#include "gem5/gem5_harness.h"
#endif

int INPUT_SIZE = sizeof(struct bench_args_t);

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  char* host_input = malloc_aligned_memcpy(&args->input, sizeof(args->input));
  int32_t* host_n_matches = malloc_aligned_memcpy(&args->n_matches, sizeof(args->n_matches));
  // pattern and kmpNext are mapped to registers (too small for SRAM), so they
  // are not DMA loaded. As a result, the accel_xyz arrays needs to have the
  // correct values from the beginning.
  char* accel_pattern = malloc_aligned_memcpy(&args->pattern, sizeof(args->pattern));
  int32_t* accel_kmpNext = malloc_aligned_memcpy(&args->kmpNext, sizeof(args->kmpNext));
  char* accel_input = malloc_aligned(sizeof(args->input));
  int32_t* accel_n_matches = calloc_aligned(sizeof(args->n_matches));
#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      MACHSUITE_KMP_KMP, "host_input", host_input, sizeof(args->input));
  mapArrayToAccelerator(
      MACHSUITE_KMP_KMP, "host_n_matches", host_n_matches, sizeof(args->n_matches));
  invokeAcceleratorAndBlock(MACHSUITE_KMP_KMP);
#else
  kmp(host_input, host_n_matches,
      accel_pattern, accel_input, accel_kmpNext, accel_n_matches);
#endif
  memcpy(&args->n_matches, host_n_matches, sizeof(int32_t));
  free(host_input);
  free(host_n_matches);
  free(accel_pattern);
  free(accel_input);
  free(accel_kmpNext);
  free(accel_n_matches);
}

/* Input format:
%% Section 1
char[PATTERN_SIZE]: pattern
%% Section 2
char[STRING_SIZE]: text
*/

void input_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_string(s, data->pattern, PATTERN_SIZE);

  s = find_section_start(p,2);
  parse_string(s, data->input, STRING_SIZE);
}

void data_to_input(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  write_string(fd, data->pattern, PATTERN_SIZE);

  write_section_header(fd);
  write_string(fd, data->input, STRING_SIZE);
}

/* Output format:
%% Section 1
int[1]: number of matches
*/

void output_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_int32_t_array(s, data->n_matches, 1);
}

void data_to_output(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd); // No section header
  write_int32_t_array(fd, data->n_matches, 1);
}

int check_data( void *vdata, void *vref ) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  struct bench_args_t *ref = (struct bench_args_t *)vref;
  int has_errors = 0;

  has_errors |= (data->n_matches[0] != ref->n_matches[0]);

  // Return true if it's correct.
  return !has_errors;
}
