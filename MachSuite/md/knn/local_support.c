#include "md.h"
#include <string.h>

#ifdef GEM5_HARNESS
#include "gem5/gem5_harness.h"
#endif

int INPUT_SIZE = sizeof(struct bench_args_t);

#define EPSILON ((TYPE)1.0e-6)

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  TYPE* host_force_x = malloc_aligned_memcpy(&args->force_x, sizeof(args->force_x));
  TYPE* host_force_y = malloc_aligned_memcpy(&args->force_y, sizeof(args->force_y));
  TYPE* host_force_z = malloc_aligned_memcpy(&args->force_z, sizeof(args->force_z));
  TYPE* host_position_x = malloc_aligned_memcpy(&args->position_x, sizeof(args->position_x));
  TYPE* host_position_y = malloc_aligned_memcpy(&args->position_y, sizeof(args->position_y));
  TYPE* host_position_z = malloc_aligned_memcpy(&args->position_z, sizeof(args->position_z));
  int32_t* host_NL= malloc_aligned_memcpy(&args->NL, sizeof(args->NL));
  TYPE* accel_force_x = calloc_aligned(sizeof(args->force_x));
  TYPE* accel_force_y = calloc_aligned(sizeof(args->force_y));
  TYPE* accel_force_z = calloc_aligned(sizeof(args->force_z));
  TYPE* accel_position_x = malloc_aligned(sizeof(args->position_x));
  TYPE* accel_position_y = malloc_aligned(sizeof(args->position_y));
  TYPE* accel_position_z = malloc_aligned(sizeof(args->position_z));
  int32_t* accel_NL = malloc_aligned(sizeof(args->NL));
#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      MACHSUITE_MD_KNN, "host_force_x", host_force_x, sizeof(args->force_x));
  mapArrayToAccelerator(
      MACHSUITE_MD_KNN, "host_force_y", host_force_y, sizeof(args->force_y));
  mapArrayToAccelerator(
      MACHSUITE_MD_KNN, "host_force_z", host_force_z, sizeof(args->force_z));
  mapArrayToAccelerator(
      MACHSUITE_MD_KNN, "host_position_x", host_position_x, sizeof(args->position_x));
  mapArrayToAccelerator(
      MACHSUITE_MD_KNN, "host_position_y", host_position_y, sizeof(args->position_y));
  mapArrayToAccelerator(
      MACHSUITE_MD_KNN, "host_position_z", host_position_z, sizeof(args->position_z));
  mapArrayToAccelerator(
      MACHSUITE_MD_KNN, "host_NL", host_NL, sizeof(args->NL));
  invokeAcceleratorAndBlock(MACHSUITE_MD_KNN);
#else
  md_kernel(host_force_x, host_force_y, host_force_z,
            host_position_x, host_position_y, host_position_z, host_NL,
            accel_force_x, accel_force_y, accel_force_z,
            accel_position_x, accel_position_y, accel_position_z, accel_NL);
#endif
  memcpy(&args->force_x, host_force_x, sizeof(args->force_x));
  memcpy(&args->force_y, host_force_y, sizeof(args->force_y));
  memcpy(&args->force_z, host_force_z, sizeof(args->force_z));
  free(host_force_x);
  free(host_force_y);
  free(host_force_z);
  free(host_position_x);
  free(host_position_y);
  free(host_position_z);
  free(host_NL);
  free(accel_force_x);
  free(accel_force_y);
  free(accel_force_z);
  free(accel_position_x);
  free(accel_position_y);
  free(accel_position_z);
  free(accel_NL);
}

/* Input format:
%% Section 1
TYPE[nAtoms]: x positions
%% Section 2
TYPE[nAtoms]: y positions
%% Section 3
TYPE[nAtoms]: z positions
%% Section 4
int32_t[nAtoms*maxNeighbors]: neighbor list
*/

void input_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,TYPE,_array)(s, data->position_x, nAtoms);

  s = find_section_start(p,2);
  STAC(parse_,TYPE,_array)(s, data->position_y, nAtoms);

  s = find_section_start(p,3);
  STAC(parse_,TYPE,_array)(s, data->position_z, nAtoms);

  s = find_section_start(p,4);
  parse_int32_t_array(s, data->NL, nAtoms*maxNeighbors);

}

void data_to_input(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->position_x, nAtoms);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->position_y, nAtoms);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->position_z, nAtoms);

  write_section_header(fd);
  write_int32_t_array(fd, data->NL, nAtoms*maxNeighbors);

}

/* Output format:
%% Section 1
TYPE[nAtoms]: new x force
%% Section 2
TYPE[nAtoms]: new y force
%% Section 3
TYPE[nAtoms]: new z force
*/

void output_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);

  s = find_section_start(p,1);
  STAC(parse_,TYPE,_array)(s, data->force_x, nAtoms);

  s = find_section_start(p,2);
  STAC(parse_,TYPE,_array)(s, data->force_y, nAtoms);

  s = find_section_start(p,3);
  STAC(parse_,TYPE,_array)(s, data->force_z, nAtoms);

}

void data_to_output(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->force_x, nAtoms);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->force_y, nAtoms);

  write_section_header(fd);
  STAC(write_,TYPE,_array)(fd, data->force_z, nAtoms);
}

int check_data( void *vdata, void *vref ) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  struct bench_args_t *ref = (struct bench_args_t *)vref;
  int has_errors = 0;
  int i;
  TYPE diff_x, diff_y, diff_z;

  for( i=0; i<nAtoms; i++ ) {
    diff_x = data->force_x[i] - ref->force_x[i];
    diff_y = data->force_y[i] - ref->force_y[i];
    diff_z = data->force_z[i] - ref->force_z[i];
    has_errors |= (diff_x<-EPSILON) || (EPSILON<diff_x);
    has_errors |= (diff_y<-EPSILON) || (EPSILON<diff_y);
    has_errors |= (diff_z<-EPSILON) || (EPSILON<diff_z);
  }

  // Return true if it's correct.
  return !has_errors;
}
