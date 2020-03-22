#include "bfs.h"
#include <string.h>
#include <unistd.h>

#ifdef GEM5_HARNESS
#include "gem5/gem5_harness.h"
#endif

int INPUT_SIZE = sizeof(struct bench_args_t);

void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  node_t* host_nodes = malloc_aligned_memcpy(&args->nodes, sizeof(args->nodes));
  edge_t* host_edges = malloc_aligned_memcpy(&args->edges, sizeof(args->edges));
  level_t* host_level = malloc_aligned_memcpy(&args->level, sizeof(args->level));
  edge_index_t* host_level_counts = calloc_aligned(sizeof(args->level_counts));
  node_t* accel_nodes = malloc_aligned(sizeof(args->nodes));
  edge_t* accel_edges = malloc_aligned(sizeof(args->edges));
  level_t* accel_level = malloc_aligned(sizeof(args->level));
  edge_index_t* accel_level_counts = calloc_aligned(sizeof(args->level_counts));
#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      MACHSUITE_BFS_QUEUE, "host_nodes", host_nodes, sizeof(args->nodes));
  mapArrayToAccelerator(
      MACHSUITE_BFS_QUEUE, "host_edges", host_edges, sizeof(args->edges));
  mapArrayToAccelerator(
      MACHSUITE_BFS_QUEUE, "host_level", host_level, sizeof(args->level));
  mapArrayToAccelerator(
      MACHSUITE_BFS_QUEUE, "host_level_counts", host_level_counts, sizeof(args->level_counts));
  invokeAcceleratorAndBlock(MACHSUITE_BFS_QUEUE);
#else
  bfs(host_nodes, host_edges, host_level, host_level_counts,
      accel_nodes, accel_edges, accel_level, accel_level_counts,
      args->starting_node);
#endif
  memcpy(&args->level_counts, host_level_counts, sizeof(args->level_counts));
  free(host_nodes);
  free(host_edges);
  free(host_level);
  free(host_level_counts);
  free(accel_nodes);
  free(accel_edges);
  free(accel_level);
  free(accel_level_counts);
}

/* Input format:
%% Section 1
uint64_t[1]: starting node
%% Section 2
uint64_t[N_NODES*2]: node structures (start and end indices of edge lists)
%% Section 3
uint64_t[N_EDGES]: edges structures (just destination node id)
*/

void input_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  uint64_t *nodes;
  int64_t i;

  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Max-ify levels
  for(i=0; i<N_NODES; i++) {
    data->level[i]=MAX_LEVEL;
  }
  // Load input string
  p = readfile(fd);
  // Section 1: starting node
  s = find_section_start(p,1);
  parse_uint64_t_array(s, &data->starting_node, 1);

  // Section 2: node structures
  s = find_section_start(p,2);
  nodes = (uint64_t *)malloc(N_NODES*2*sizeof(uint64_t));
  parse_uint64_t_array(s, nodes, N_NODES*2);
  for(i=0; i<N_NODES; i++) {
    data->nodes[i].edge_begin = nodes[2*i];
    data->nodes[i].edge_end = nodes[2*i+1];
  }
  free(nodes);
  // Section 3: edge structures
  s = find_section_start(p,3);
  parse_uint64_t_array(s, (uint64_t *)(data->edges), N_EDGES);
}

void data_to_input(int fd, void *vdata) {
  uint64_t *nodes;
  int64_t i;

  struct bench_args_t *data = (struct bench_args_t *)vdata;
  // Section 1: starting node
  write_section_header(fd);
  write_uint64_t_array(fd, &data->starting_node, 1);
  // Section 2: node structures
  write_section_header(fd);
  nodes = (uint64_t *)malloc(N_NODES*2*sizeof(uint64_t));
  for(i=0; i<N_NODES; i++) {
    nodes[2*i]  = data->nodes[i].edge_begin;
    nodes[2*i+1]= data->nodes[i].edge_end;
  }
  write_uint64_t_array(fd, nodes, N_NODES*2);
  free(nodes);
  // Section 3: edge structures
  write_section_header(fd);
  write_uint64_t_array(fd, (uint64_t *)(&data->edges), N_EDGES);
}

/* Output format:
%% Section 1
uint64_t[N_LEVELS]: horizon counts
*/

void output_to_data(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  char *p, *s;
  // Zero-out everything.
  memset(vdata,0,sizeof(struct bench_args_t));
  // Load input string
  p = readfile(fd);
  // Section 1: horizon counts
  s = find_section_start(p,1);
  parse_uint64_t_array(s, data->level_counts, N_LEVELS);
}

void data_to_output(int fd, void *vdata) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  // Section 1
  write_section_header(fd);
  write_uint64_t_array(fd, data->level_counts, N_LEVELS);
}

int check_data( void *vdata, void *vref ) {
  struct bench_args_t *data = (struct bench_args_t *)vdata;
  struct bench_args_t *ref = (struct bench_args_t *)vref;
  int has_errors = 0;
  int i;

  // Check that the horizons have the same number of nodes
  for(i=0; i<N_LEVELS; i++) {
    has_errors |= (data->level_counts[i]!=ref->level_counts[i]);
  }

  // Return true if it's correct.
  return !has_errors;
}
