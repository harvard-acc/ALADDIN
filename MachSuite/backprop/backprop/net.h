#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define TYPE double

//Number of times you train the network with the same data set
#define EPOCS 100

//Number of training data
#define NUM_TRAIN 100

#define MAX_ROWS 10
#define MAX_COLS 10

#define NUM_LAYERS 3

//Number of acspects for each training data
#define SIZE_IN 4

//Classification catagories (output nodes)
#define SIZE_OUT 10

//Learning Rate
#define N 0.5

//Momentum
#define M 0.1

#ifdef GEM5_HARNESS
#include "gem5/aladdin_sys_connection.h"
#include "gem5/aladdin_sys_constants.h"
#endif

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

static const int layer_size[] = {SIZE_IN, MAX_COLS, SIZE_OUT};

void backprop(TYPE weights[NUM_LAYERS - 1][MAX_ROWS][MAX_COLS],
        TYPE inputs[NUM_TRAIN][SIZE_IN],
        TYPE targets[NUM_TRAIN][SIZE_OUT]);

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
        TYPE weights[NUM_LAYERS - 1][MAX_ROWS][MAX_COLS];
        TYPE inputs[NUM_TRAIN][SIZE_IN];
        TYPE targets[NUM_TRAIN][SIZE_OUT];
};
int INPUT_SIZE = sizeof(struct bench_args_t);


void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;

#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      MACHSUITE_BACKPROP_BACKPROP, "weights", (void*)&args->weights,
                                              sizeof(args->weights));
  mapArrayToAccelerator(
      MACHSUITE_BACKPROP_BACKPROP, "inputs", (void*)&args->inputs,
                                              sizeof(args->inputs));
  mapArrayToAccelerator(
      MACHSUITE_BACKPROP_BACKPROP, "targets", (void*)&args->targets,
                                              sizeof(args->targets));
  invokeAcceleratorAndBlock(MACHSUITE_BACKPROP_BACKPROP);
#else
  backprop( args->weights, args->inputs, args->targets );
#endif
}

////////////////////////////////////////////////////////////////////////////////
