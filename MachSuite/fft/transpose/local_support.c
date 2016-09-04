#include "fft.h"
#include <string.h>

int INPUT_SIZE = sizeof(struct bench_args_t);

#define EPSILON ((TYPE)1.0e-6)

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  fft1D_512( args->work_x, args->work_y);
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
