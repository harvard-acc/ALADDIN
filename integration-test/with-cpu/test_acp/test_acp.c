/* Test stores performed through ACP.
 *
 * The values stored should be visible to the CPU.
 */

#include <stdio.h>
#include "aladdin_sys_connection.h"
#include "aladdin_sys_constants.h"

#define TYPE int
#define CACHELINE_SIZE 64

int test_stores(TYPE* store_vals, TYPE* store_loc, int num_vals) {
  int num_failures = 0;
  for (int i = 0; i < num_vals; i++) {
    if (store_loc[i] != store_vals[i]) {
      fprintf(stdout, "FAILED: store_loc[%d] = %d, should be %d\n",
                               i, store_loc[i], store_vals[i]);
      num_failures++;
    }
  }
  return num_failures;
}


// Read values from store_vals and copy them into store_loc.
void store_kernel(TYPE* store_vals, TYPE* store_loc, int num_vals) {
  loop: for (int i = 0; i < num_vals; i++)
    store_loc[i] = store_vals[i];
}

int main() {
  const int num_vals = 2048;
  TYPE* store_vals = NULL;
  TYPE* store_loc = NULL;
  int err = posix_memalign((void**)&store_loc, CACHELINE_SIZE, num_vals * sizeof(TYPE));
  err |= posix_memalign((void**)&store_vals, CACHELINE_SIZE, num_vals * sizeof(TYPE));
  if (err) {
    fprintf(stderr, "Unable to allocate aligned memory! Quitting test.\n");
    return -1;
  }

  for (int i = 0; i < num_vals; i++) {
    store_vals[i] = i;
    store_loc[i] = -1;
  }

#if defined(LLVM_TRACE) || !defined(GEM5_HARNESS)
  store_kernel(store_vals, store_loc, num_vals);
#else
  mapArrayToAccelerator(
      INTEGRATION_TEST, "store_vals", &(store_vals[0]), num_vals * sizeof(int));
  mapArrayToAccelerator(
      INTEGRATION_TEST, "store_loc", &(store_loc[0]), num_vals * sizeof(int));

  fprintf(stdout, "Invoking accelerator!\n");
  invokeAcceleratorAndBlock(INTEGRATION_TEST);
  fprintf(stdout, "Accelerator finished!\n");
#endif

  int num_failures = test_stores(store_vals, store_loc, num_vals);
  if (num_failures != 0) {
    fprintf(stdout, "Test failed with %d errors.", num_failures);
    return -1;
  }
  fprintf(stdout, "Test passed!\n");
  free(store_loc);
  free(store_vals);
  return 0;
}
