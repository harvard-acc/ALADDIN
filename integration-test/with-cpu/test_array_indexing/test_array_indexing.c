/* Examples of how to do multi-dimensional array indexing.
 *
 * The suggested way is to pass the original plain pointer to the beginning of
 * the array into the function and perform a cast to the desired dimensionality
 * inside that function. Note the tricky syntax required to do so, and the fact
 * that in such cases, we don't specify the size of the last dimension (since
 * there is no multiplication by any value that needs to be done on that last
 * index).
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "gem5_harness.h"

#define TYPE int
#define CACHELINE_SIZE 32
#define L1_SZ 8
#define L2_SZ 16
#define L3_SZ 32

int validate(TYPE* array) {
  int num_failures = 0;
  TYPE (*array_3d)[L1_SZ][L2_SZ] = (TYPE(*)[L1_SZ][L2_SZ]) array;

  for (int i = 0; i < L1_SZ; i++) {
    for (int j = 0; j < L2_SZ; j++) {
      for (int k = 0; k < L3_SZ; k++) {
        if (array_3d[i][j][k] != (i * (L1_SZ * L2_SZ) + j * (L2_SZ) + k)) {
          num_failures++;
          printf("array_3d[%d][%d][%d] = %d\n", i, j, k, array_3d[i][j][k]);
        }
      }
    }
  }
  return num_failures;
}

void kernel_2d(TYPE* array_host, TYPE* array_acc) {
  dmaLoad(array_acc, array_host, L1_SZ * L2_SZ * sizeof(TYPE));
  TYPE (*array_2d)[L1_SZ] = (TYPE(*)[L1_SZ]) array_acc;

  loop1: for (int i = 0; i < L1_SZ; i++) {
    loop2: for (int j = 0; j < L2_SZ; j++) {
      array_2d[i][j] += 3;
    }
  }

  dmaStore(array_host, array_acc, L1_SZ * L2_SZ * sizeof(TYPE));
}

void kernel_3d(TYPE* array_host, TYPE* array_acc) {
  dmaLoad(array_acc, array_host, L1_SZ * L2_SZ * L3_SZ * sizeof(TYPE));
  TYPE (*array_3d)[L1_SZ][L2_SZ] = (TYPE(*)[L1_SZ][L2_SZ]) array_acc;

  loop1: for (int i = 0; i < L1_SZ; i++) {
    loop2: for (int j = 0; j < L2_SZ; j++) {
      loop3: for (int k = 0; k < L3_SZ; k++) {
        array_3d[i][j][k] = i * (L1_SZ * L2_SZ) + j * (L2_SZ) + k;
      }
    }
  }

  dmaStore(array_host, array_acc, L1_SZ * L2_SZ * L3_SZ * sizeof(TYPE));
}

// Create a multi-dimensional array with size determined at runtime (possible
// in C99).
void kernel_2d_dyn(TYPE* array_host, TYPE* array_acc, int dynamic_size) {
  dmaLoad(array_acc, array_host, dynamic_size * dynamic_size * sizeof(TYPE));
  TYPE (*array_2d)[dynamic_size] = (TYPE(*)[dynamic_size]) array_acc;

  loop1: for (int i = 0; i < dynamic_size; i++) {
    loop2: for (int j = 0; j < dynamic_size; j++) {
        array_2d[i][j] += 5;
    }
  }

  dmaStore(array_host, array_acc, dynamic_size * dynamic_size * sizeof(TYPE));
}

void kernel(TYPE* array_host, TYPE* array_acc, int dynamic_size) {
  kernel_2d(array_host, array_acc);
  dmaFence();
  kernel_2d_dyn(array_host, array_acc, dynamic_size);
  dmaFence();
  kernel_3d(array_host, array_acc);
}

int main() {
  const size_t num_vals = L1_SZ * L2_SZ * L3_SZ;
  TYPE* array_host = NULL;
  TYPE* array_acc = NULL;
  int err = posix_memalign(
      (void**)&array_host, CACHELINE_SIZE, num_vals * sizeof(TYPE));
  err |= posix_memalign(
      (void**)&array_acc, CACHELINE_SIZE, num_vals * sizeof(TYPE));
  if (err) {
    fprintf(stderr, "Unable to allocate aligned memory! Quitting test.\n");
    return -1;
  }

  for (int i = 0; i < num_vals; i++) {
    array_host[i] = -1;
  }

#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      INTEGRATION_TEST, "array_host", array_host, num_vals * sizeof(TYPE));

  fprintf(stdout, "Invoking accelerator!\n");
  invokeAcceleratorAndBlock(INTEGRATION_TEST);
  fprintf(stdout, "Accelerator finished!\n");
#else
  kernel(array_host, array_acc, 16);
#endif

  int num_failures = validate(array_host);
  if (num_failures != 0) {
    fprintf(stdout, "Test failed with %d errors.\n", num_failures);
    return -1;
  }
  fprintf(stdout, "Test passed!\n");

  free(array_host);
  free(array_acc);

  return 0;
}
