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

void kernel_2d(TYPE* array) {
#ifdef DMA_MODE
  dmaLoad(array, array, L1_SZ * L2_SZ * sizeof(TYPE));
#endif
  TYPE (*array_2d)[L1_SZ] = (TYPE(*)[L1_SZ]) array;

  loop1: for (int i = 0; i < L1_SZ; i++) {
    loop2: for (int j = 0; j < L2_SZ; j++) {
      array_2d[i][j] += 3;
    }
  }

#ifdef DMA_MODE
  dmaStore(array, array, L1_SZ * L2_SZ * sizeof(TYPE));
#endif
}

void kernel_3d(TYPE* array) {
#ifdef DMA_MODE
  dmaLoad(array, array, L1_SZ * L2_SZ * L3_SZ * sizeof(TYPE));
#endif
  TYPE (*array_3d)[L1_SZ][L2_SZ] = (TYPE(*)[L1_SZ][L2_SZ]) array;

  loop1: for (int i = 0; i < L1_SZ; i++) {
    loop2: for (int j = 0; j < L2_SZ; j++) {
      loop3: for (int k = 0; k < L3_SZ; k++) {
        array_3d[i][j][k] = i * (L1_SZ * L2_SZ) + j * (L2_SZ) + k;
      }
    }
  }

#ifdef DMA_MODE
  dmaStore(array, array, L1_SZ * L2_SZ * L3_SZ * sizeof(TYPE));
#endif
}

// Create a multi-dimensional array with size determined at runtime (possible
// in C99).
void kernel_2d_dyn(TYPE* array, int dynamic_size) {
#ifdef DMA_MODE
  dmaLoad(array, array, dynamic_size * dynamic_size * sizeof(TYPE));
#endif
  TYPE (*array_2d)[dynamic_size] = (TYPE(*)[dynamic_size]) array;

  loop1: for (int i = 0; i < dynamic_size; i++) {
    loop2: for (int j = 0; j < dynamic_size; j++) {
        array_2d[i][j] += 5;
    }
  }

#ifdef DMA_MODE
  dmaStore(array, array, dynamic_size * dynamic_size * sizeof(TYPE));
#endif
}

void kernel(TYPE* array, int dynamic_size) {
  kernel_2d(array);
  kernel_2d_dyn(array, dynamic_size);
  kernel_3d(array);
}

int main() {
  const size_t num_vals = L1_SZ * L2_SZ * L3_SZ;
  TYPE* array;
  posix_memalign((void**)&array, CACHELINE_SIZE, num_vals * sizeof(TYPE));

  for (int i = 0; i < num_vals; i++) {
    array[i] = -1;
  }

#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      INTEGRATION_TEST, "array", array, num_vals * sizeof(TYPE));

  fprintf(stdout, "Invoking accelerator!\n");
  invokeAcceleratorAndBlock(INTEGRATION_TEST);
  fprintf(stdout, "Accelerator finished!\n");
#else
  kernel(array, 16);
#endif

  int num_failures = validate(array);
  if (num_failures != 0) {
    fprintf(stdout, "Test failed with %d errors.\n", num_failures);
    return -1;
  }
  fprintf(stdout, "Test passed!\n");

  free(array);

  return 0;
}
