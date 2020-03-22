#include "nw.h"
#include <string.h>

#ifdef GEM5_HARNESS
#include "gem5/gem5_harness.h"
#endif

int INPUT_SIZE = sizeof(struct bench_args_t);

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  char* host_SEQA = malloc_aligned_memcpy(&args->seqA, sizeof(args->seqA));
  char* host_SEQB = malloc_aligned_memcpy(&args->seqB, sizeof(args->seqB));
  char* host_alignedA = malloc_aligned_memcpy(&args->alignedA, sizeof(args->alignedA));
  char* host_alignedB = malloc_aligned_memcpy(&args->alignedB, sizeof(args->alignedB));
  char* accel_SEQA = malloc_aligned(sizeof(args->seqA));
  char* accel_SEQB = malloc_aligned(sizeof(args->seqB));
  char* accel_alignedA = calloc_aligned(sizeof(args->alignedA));
  char* accel_alignedB = calloc_aligned(sizeof(args->alignedB));
  int* accel_M = malloc_aligned(sizeof(args->M));
  char* accel_ptr = malloc_aligned(sizeof(args->ptr));
#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      MACHSUITE_NW_NW, "host_SEQA", host_SEQA, sizeof(args->seqA));
  mapArrayToAccelerator(
      MACHSUITE_NW_NW, "host_SEQB", host_SEQB, sizeof(args->seqB));
  mapArrayToAccelerator(
      MACHSUITE_NW_NW, "host_alignedA", host_alignedA,
      sizeof(args->alignedA));
  mapArrayToAccelerator(
      MACHSUITE_NW_NW, "host_alignedB", host_alignedB,
      sizeof(args->alignedB));
  invokeAcceleratorAndBlock(MACHSUITE_NW_NW);
#else
  needwun(
      host_SEQA, host_SEQB, host_alignedA, host_alignedB,
      accel_SEQA, accel_SEQB, accel_alignedA, accel_alignedB, accel_M, accel_ptr);
#endif
  memcpy(&args->alignedA, host_alignedA, sizeof(args->alignedA));
  memcpy(&args->alignedB, host_alignedB, sizeof(args->alignedB));
  free(host_SEQA);
  free(host_SEQB);
  free(host_alignedA);
  free(host_alignedB);
  free(accel_SEQA);
  free(accel_SEQB);
  free(accel_alignedA);
  free(accel_alignedB);
  free(accel_M);
  free(accel_ptr);
}

/* Input format:
%% Section 1
char[]: sequence A
%% Section 2
char[]: sequence B
*/

void input_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_string(s, data->seqA, ALEN);

  s = find_section_start(p,2);
  parse_string(s, data->seqB, BLEN);

}

void data_to_input(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  write_string(fd, data->seqA, ALEN);

  write_section_header(fd);
  write_string(fd, data->seqB, BLEN);

  write_section_header(fd);
}

/* Output format:
%% Section 1
char[sum_size]: aligned sequence A
%% Section 2
char[sum_size]: aligned sequence B
*/

void output_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_string(s, data->alignedA, ALEN+BLEN);

  s = find_section_start(p,2);
  parse_string(s, data->alignedB, ALEN+BLEN);
}

void data_to_output(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  write_string(fd, data->alignedA, ALEN+BLEN);

  write_section_header(fd);
  write_string(fd, data->alignedB, ALEN+BLEN);

  write_section_header(fd);
}

int check_data( void *vdata, void *vref ) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  struct bench_args_t *ref = (struct bench_args_t *)vref;
  int has_errors = 0;

  has_errors |= memcmp(data->alignedA, ref->alignedA, ALEN+BLEN);
  has_errors |= memcmp(data->alignedB, ref->alignedB, ALEN+BLEN);

  // Return true if it's correct.
  return !has_errors;
}
