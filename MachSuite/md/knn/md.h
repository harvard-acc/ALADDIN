/*
Implemenataion based on:
A. Danalis, G. Marin, C. McCurdy, J. S. Meredith, P. C. Roth, K. Spafford, V. Tipparaju, and J. S. Vetter.
The scalable heterogeneous computing (shoc) benchmark suite.
In Proceedings of the 3rd Workshop on General-Purpose Computation on Graphics Processing Units, 2010.
*/

#include <stdlib.h>
#include <stdio.h>
#include "support.h"

#define TYPE double

// Problem Constants
#define nAtoms        256
#define maxNeighbors  16
// LJ coefficients
#define lj1           1.5
#define lj2           2.0

void md_kernel(TYPE* host_force_x,
               TYPE* host_force_y,
               TYPE* host_force_z,
               TYPE* host_position_x,
               TYPE* host_position_y,
               TYPE* host_position_z,
               int32_t* host_NL,
               TYPE* force_x,
               TYPE* force_y,
               TYPE* force_z,
               TYPE* position_x,
               TYPE* position_y,
               TYPE* position_z,
               int32_t* NL);
////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
  TYPE force_x[nAtoms];
  TYPE force_y[nAtoms];
  TYPE force_z[nAtoms];
  TYPE position_x[nAtoms];
  TYPE position_y[nAtoms];
  TYPE position_z[nAtoms];
  int32_t NL[nAtoms*maxNeighbors];
};
