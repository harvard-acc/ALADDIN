/* Test SIMD operations.
 *
 * The final values stored should be visible to the CPU.
 */

#include <stdio.h>
#include "gem5_harness.h"

// This is a vector of 8 ints (32 bytes total).
#define VECTOR_SIZE 8
typedef int scalar_t;
typedef scalar_t v8si
    __attribute__((__vector_size__(VECTOR_SIZE * sizeof(scalar_t))));

typedef v8si val_t;

#define CACHELINE_SIZE 64

int test_stores(val_t* store_val0, val_t* store_val1, val_t* store_loc, int num_vals) {
  int num_failures = 0;
  for (int i = 0; i < num_vals; i++) {
    for (int j = 0; j < VECTOR_SIZE; j++) {
      int expected = store_val0[i][j] + store_val1[i][j];
      if (store_loc[i][j] != expected) {
        fprintf(stdout, "FAILED: store_loc[%d][%d] = %d, should be %d\n", i, j,
                store_loc[i][j], expected);
        num_failures++;
      }
    }
  }
  return num_failures;
}


void store_kernel(val_t* store_val0,  // in scratchpads.
                  val_t* store_val1,  // from cache.
                  val_t* store_loc,   // to ACP.
                  int num_vals) {
  dmaLoad(store_val0, store_val0, num_vals * sizeof(val_t));
  loop: for (int i = 0; i < num_vals; i++)
    store_loc[i] = store_val0[i] + store_val1[i];
}

int main() {
  const int num_vals = 256;
  val_t *store_val0 = NULL, *store_val1 = NULL, *store_loc = NULL;
  int err = posix_memalign((void**)&store_loc, CACHELINE_SIZE, num_vals * sizeof(val_t));
  err |= posix_memalign((void**)&store_val0, CACHELINE_SIZE, num_vals * sizeof(val_t));
  err |= posix_memalign((void**)&store_val1, CACHELINE_SIZE, num_vals * sizeof(val_t));
  if (err) {
    fprintf(stderr, "Unable to allocate aligned memory! Quitting test.\n");
    return -1;
  }

  for (int i = 0; i < num_vals; i++) {
    for (int j = 0; j < VECTOR_SIZE; j++) {
      store_val0[i][j] = i;
      store_val1[i][j] = 2 * i;
      store_loc[i][j] = -1;
    }
  }

#if defined(LLVM_TRACE) || !defined(GEM5_HARNESS)
  store_kernel(store_val0, store_val1, store_loc, num_vals);
#else
  mapArrayToAccelerator(
      INTEGRATION_TEST, "store_val0", store_val0, num_vals * sizeof(val_t));
  mapArrayToAccelerator(
      INTEGRATION_TEST, "store_val1", store_val1, num_vals * sizeof(val_t));
  mapArrayToAccelerator(
      INTEGRATION_TEST, "store_loc", store_loc, num_vals * sizeof(val_t));

  fprintf(stdout, "Invoking accelerator!\n");
  invokeAcceleratorAndBlock(INTEGRATION_TEST);
  fprintf(stdout, "Accelerator finished!\n");
#endif

  int num_failures = test_stores(store_val0, store_val1, store_loc, num_vals);
  if (num_failures != 0) {
    fprintf(stdout, "Test failed with %d errors.", num_failures);
    return -1;
  }
  fprintf(stdout, "Test passed!\n");
  free(store_loc);
  free(store_val0);
  free(store_val1);
  return 0;
}
