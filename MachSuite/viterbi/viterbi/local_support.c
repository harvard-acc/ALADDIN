#include "viterbi.h"
#include <string.h>

int INPUT_SIZE = sizeof(struct bench_args_t);

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  viterbi( args->obs, args->init, args->transition, args->emission, args->path );
}

/* Input format:
%% Section 1
tok_t[N_OBS]: observation vector
%% Section 2
prob_t[N_STATES]: initial state probabilities
%% Section 3
prob_t[N_STATES*N_STATES]: transition matrix
%% Section 4
prob_t[N_STATES*N_TOKENS]: emission matrix
*/

void input_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_uint8_t_array(s, data->obs, N_OBS);

  s = find_section_start(p,2);
  STAC(parse_,TYPE,_array)(s, data->init, N_STATES);

  s = find_section_start(p,3);
  STAC(parse_,TYPE,_array)(s, data->transition, N_STATES*N_STATES);

  s = find_section_start(p,4);
  STAC(parse_,TYPE,_array)(s, data->emission, N_STATES*N_TOKENS);
}

void data_to_input(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  write_uint8_t_array(fd, data->obs, N_OBS);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->init, N_STATES);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->transition, N_STATES*N_STATES);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->emission, N_STATES*N_TOKENS);
}

/* Output format:
%% Section 1
uint8_t[N_OBS]: most likely state chain
*/

void output_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  parse_uint8_t_array(s, data->path, N_OBS);
}

void data_to_output(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  write_uint8_t_array(fd, data->path, N_OBS);
}

int check_data( void *vdata, void *vref ) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  struct bench_args_t *ref = (struct bench_args_t *)vref;
  int has_errors = 0;
  int i;

  for(i=0; i<N_OBS; i++) {
    has_errors |= (data->path[i]!=ref->path[i]);
  }

  // Return true if it's correct.
  return !has_errors;
}
