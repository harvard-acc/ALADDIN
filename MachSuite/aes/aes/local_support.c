#include "aes.h"
#include <string.h>
#ifdef GEM5_HARNESS
#include "gem5/gem5_harness.h"
#endif

int INPUT_SIZE = sizeof(struct bench_args_t);

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  // Copy args into separate host arrays.
  aes256_context* host_ctx = malloc_aligned_memcpy(&args->ctx, sizeof(args->ctx));
  uint8_t* host_k = malloc_aligned_memcpy(&args->k, sizeof(args->k));
  uint8_t* host_buf = malloc_aligned_memcpy(&args->buf, sizeof(args->buf));
  // Allocate memory for the accelerator arrays.
  aes256_context* accel_ctx = malloc_aligned(sizeof(args->ctx));
  uint8_t* accel_k = malloc_aligned(sizeof(args->k));
  uint8_t* accel_buf = malloc_aligned(sizeof(args->buf));
#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      MACHSUITE_AES_AES, "host_ctx", host_ctx, sizeof(args->ctx));
  mapArrayToAccelerator(
      MACHSUITE_AES_AES, "host_k", host_k, sizeof(args->k));
  mapArrayToAccelerator(
      MACHSUITE_AES_AES, "host_buf", host_buf, sizeof(args->buf));
  invokeAcceleratorAndBlock(MACHSUITE_AES_AES);
#else
  aes256_encrypt_ecb(host_ctx, host_k, host_buf,
                     accel_ctx, accel_k, accel_buf);
#endif
  memcpy(&args->buf, host_buf, sizeof(args->buf));
  free(host_ctx);
  free(host_k);
  free(host_buf);
  free(accel_ctx);
  free(accel_k);
  free(accel_buf);
}

/* Input format:
%%: Section 1
uint8_t[32]: key
%%: Section 2
uint8_t[16]: input-text
*/

void input_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);
  // Section 1: key
  s = find_section_start(p,1);
  parse_uint8_t_array(s, data->k, 32);
  // Section 2: input-text
  s = find_section_start(p,2);
  parse_uint8_t_array(s, data->buf, 16);
}

void data_to_input(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  // Section 1
  write_section_header(fd);
  write_uint8_t_array(fd, data->k, 32);
  // Section 2
  write_section_header(fd);
  write_uint8_t_array(fd, data->buf, 16);
}

/* Output format:
%% Section 1
uint8_t[16]: output-text
*/

void output_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);
  // Section 1: output-text
  s = find_section_start(p,1);
  parse_uint8_t_array(s, data->buf, 16);
}

void data_to_output(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  // Section 1
  write_section_header(fd);
  write_uint8_t_array(fd, data->buf, 16);
}

int check_data( void *vdata, void *vref ) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  struct bench_args_t *ref = (struct bench_args_t *)vref;
  int has_errors = 0;

  // Exact compare encrypted output buffers
  has_errors |= memcmp(&data->buf, &ref->buf, 16*sizeof(uint8_t));

  // Return true if it's correct.
  return !has_errors;
}
