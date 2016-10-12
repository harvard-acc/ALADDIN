#include "FIXME.h"
#include <string.h>

int INPUT_SIZE = sizeof(struct bench_args_t);

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  FIXME( &(args->ctx), args->k, args->buf );
}

/* Input format:
%% Section 1
FIXME type[SIZE]: FIXME description
...
*/

void input_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_FIXME_array(s, data->FIXME_attribute, FIXME_size);
  // FIXME ...
}

void data_to_input(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  write_FIXME_array(fd, data->FIXME_attribute, FIXME_size);
  // FIXME ...
}

/* Output format:
%% Section 1
FIXME type[SIZE]: FIXME description
...
*/

void output_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_FIXME_array(s, data->FIXME_attribute, FIXME_size);
  // FIXME ...
}

void data_to_output(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  write_FIXME_array(fd, data->FIXME_attribute, FIXME_size);
  // FIXME ...
}

int check_data( void *vdata, void *vref ) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  struct bench_args_t *ref = (struct bench_args_t *)vref;
  int has_errors = 0;

  // FIXME
  // example of direct memory (string) compare
  //has_errors |= memcmp(&data->FIXME_attr, &ref->FIXME_attr, FIXME_size);
  // example of element compare
  //for(int i=0; i<FIXME_size; i++) {
  //  has_errors |= (data->FIXME_attr[i]!=ref->FIXME_attr[i]);
  //}

  // Return true if it's correct.
  return !has_errors;
}
