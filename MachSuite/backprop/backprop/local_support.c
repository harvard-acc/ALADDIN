#include "backprop.h"
#include <string.h>

int INPUT_SIZE = sizeof(struct bench_args_t);

#define EPSILON (1.0e-6)

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  backprop( args->weights1, args->weights2, args->weights3,
            args->biases1,  args->biases2,  args->biases3,
            args->training_data, args->training_targets );
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
  STAC(parse_,TYPE,_array)(s, data->weights1, input_dimension*nodes_per_layer);

  s = find_section_start(p,2);
  STAC(parse_,TYPE,_array)(s, data->weights2, nodes_per_layer*nodes_per_layer);

  s = find_section_start(p,3);
  STAC(parse_,TYPE,_array)(s, data->weights3, nodes_per_layer*possible_outputs);

  s = find_section_start(p,4);
  STAC(parse_,TYPE,_array)(s, data->biases1, nodes_per_layer);

  s = find_section_start(p,5);
  STAC(parse_,TYPE,_array)(s, data->biases2, nodes_per_layer);

  s = find_section_start(p,6);
  STAC(parse_,TYPE,_array)(s, data->biases3, possible_outputs);

  s = find_section_start(p,7);
  STAC(parse_,TYPE,_array)(s, data->training_data, training_sets*input_dimension);

  s = find_section_start(p,8);
  STAC(parse_,TYPE,_array)(s, data->training_targets, training_sets*possible_outputs);
}

void data_to_input(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->weights1, input_dimension*nodes_per_layer);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->weights2, nodes_per_layer*nodes_per_layer);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->weights3, nodes_per_layer*possible_outputs);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->biases1, nodes_per_layer);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->biases2, nodes_per_layer);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->biases3, possible_outputs);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->training_data, training_sets*input_dimension);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->training_targets, training_sets*possible_outputs);
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
  STAC(parse_,TYPE,_array)(s, data->weights1, input_dimension*nodes_per_layer);

  s = find_section_start(p,2);
  STAC(parse_,TYPE,_array)(s, data->weights2, nodes_per_layer*nodes_per_layer);

  s = find_section_start(p,3);
  STAC(parse_,TYPE,_array)(s, data->weights3, nodes_per_layer*possible_outputs);

  s = find_section_start(p,4);
  STAC(parse_,TYPE,_array)(s, data->biases1, nodes_per_layer);

  s = find_section_start(p,5);
  STAC(parse_,TYPE,_array)(s, data->biases2, nodes_per_layer);

  s = find_section_start(p,6);
  STAC(parse_,TYPE,_array)(s, data->biases3, possible_outputs);

}

void data_to_output(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->weights1, input_dimension*nodes_per_layer);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->weights2, nodes_per_layer*nodes_per_layer);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->weights3, nodes_per_layer*possible_outputs);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->biases1, nodes_per_layer);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->biases2, nodes_per_layer);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->biases3, possible_outputs);

}

int check_data( void *vdata, void *vref ) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  struct bench_args_t *ref = (struct bench_args_t *)vref;
  int has_errors = 0;
  int i, j;
  TYPE diff;

  for(i=0; i<input_dimension; i++) {
    for(j=0; j<nodes_per_layer; j++) {
      diff = data->weights1[i*nodes_per_layer + j] - ref->weights1[i*nodes_per_layer + j];
      has_errors |= (diff<-EPSILON) || (EPSILON<diff);
    }
  }
  for(i=0; i<nodes_per_layer; i++) {
    for(j=0; j<nodes_per_layer; j++) {
      diff = data->weights2[i*nodes_per_layer + j] - ref->weights2[i*nodes_per_layer + j];
      has_errors |= (diff<-EPSILON) || (EPSILON<diff);
    }
  }
  for(i=0; i<nodes_per_layer; i++) {
    for(j=0; j<possible_outputs; j++) {
      diff = data->weights3[i*possible_outputs + j] - ref->weights3[i*possible_outputs + j];
      has_errors |= (diff<-EPSILON) || (EPSILON<diff);
    }
  }
  for(i=0; i<nodes_per_layer; i++) {
    diff = data->biases1[i] - ref->biases1[i];
    has_errors |= (diff<-EPSILON) || (EPSILON<diff);
  }
  for(i=0; i<nodes_per_layer; i++) {
    diff = data->biases2[i] - ref->biases2[i];
    has_errors |= (diff<-EPSILON) || (EPSILON<diff);
  }
  for(i=0; i<possible_outputs; i++) {
    diff = data->biases3[i] - ref->biases3[i];
    has_errors |= (diff<-EPSILON) || (EPSILON<diff);
  }
  // Return true if it's correct.
  return !has_errors;
}
