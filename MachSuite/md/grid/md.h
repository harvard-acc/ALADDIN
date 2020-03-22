#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "support.h"

#define TYPE double

// Problem Constants
#define nAtoms        256
#define domainEdge    20.0
#define blockSide     4
//#define blockSide     1
#define nBlocks       (blockSide*blockSide*blockSide)
#define blockEdge     (domainEdge/((TYPE)blockSide))
// Memory Bound
// This is an artifact of using statically-allocated arrays. We'll pretend that
// it doesn't exist and instead track the actual number of points.
#define densityFactor 10
// LJ coefficients
#define lj1           1.5
#define lj2           2.0

// Some macros to convert pointers to multidimensional arrays.
//
//  If we have an array like array[5][4][3]:
//     ARRAY_3D(TYPE, output_name, array, 4, 3);
//  If we have an array like array[5][4][3][2]
//     ARRAY_4D(TYPE, output_name, array, 4, 3, 2);
#define ARRAY_2D(TYPE, output_array_name, input_array_name, DIM_1)             \
      TYPE(*output_array_name)[DIM_1] = (TYPE(*)[DIM_1])input_array_name

#define ARRAY_3D(TYPE, output_array_name, input_array_name, DIM_1, DIM_2)      \
      TYPE(*output_array_name)[DIM_1][DIM_2] =                                 \
        (TYPE(*)[DIM_1][DIM_2])input_array_name

#define ARRAY_4D(                                                              \
    TYPE, output_array_name, input_array_name, DIM_1, DIM_2, DIM_3)            \
        TYPE(*output_array_name)[DIM_1][DIM_2][DIM_3] =                        \
            (TYPE(*)[DIM_1][DIM_2][DIM_3])input_array_name

typedef struct {
  TYPE x, y, z;
} dvector_t;
typedef struct {
  int32_t x, y, z;
} ivector_t;

void md(int* host_n_points,
        dvector_t* host_force,
        dvector_t* host_position,
        int* n_points,
        dvector_t* force,
        dvector_t * position);
////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
  int32_t n_points[blockSide][blockSide][blockSide];
  dvector_t force[blockSide][blockSide][blockSide][densityFactor];
  dvector_t position[blockSide][blockSide][blockSide][densityFactor];
};
