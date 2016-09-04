#include "stencil.h"
#include <string.h>

int INPUT_SIZE = sizeof(struct bench_args_t);

#define EPSILON (1.0e-6)

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  stencil( args->orig, args->sol, args->filter );
}

/* Input format:
%% Section 1
TYPE[row_size*col_size]: input matrix
%% Section 2
TYPE[f_size]: filter coefficients
*/

void input_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,TYPE,_array)(s, data->orig, row_size*col_size);

  s = find_section_start(p,2);
  STAC(parse_,TYPE,_array)(s, data->filter, f_size);
}

void data_to_input(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->orig, row_size*col_size);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->filter, f_size);
}

/* Output format:
%% Section 1
TYPE[row_size*col_size]: solution matrix
*/

void output_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,TYPE,_array)(s, data->sol, row_size*col_size);
}

void data_to_output(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->sol, row_size*col_size);
}

int check_data( void *vdata, void *vref ) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  struct bench_args_t *ref = (struct bench_args_t *)vref;
  int has_errors = 0;
  int row, col;
  TYPE diff;

  for(row=0; row<row_size; row++) {
    for(col=0; col<col_size; col++) {
      diff = data->sol[row*col_size + col] - ref->sol[row*col_size + col];
      has_errors |= (diff<-EPSILON) || (EPSILON<diff);
    }
  }

  // Return true if it's correct.
  return !has_errors;
}
