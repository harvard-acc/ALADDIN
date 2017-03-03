/* Test stores performed in the kernel.
 *
 * This test uses both the cache and DMA to access input data.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "aladdin_sys_connection.h"
#include "aladdin_sys_constants.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

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
#ifdef DMA_MODE
  dmaLoad(&store_vals[0], 0, 4096);
  // Don't DMA store_loc! That part will be accessed via coherent memory.
#endif
  loop: for (int i = 0; i < num_vals; i++)
    store_loc[i] = store_vals[i];
#ifdef DMA_MODE
  dmaStore(&store_vals[0], 0, 4096);
#endif
}

int main() {
  const int num_vals = 1024;
  TYPE *store_vals, *store_loc;
  int err = posix_memalign(
      (void**)&store_vals, CACHELINE_SIZE, sizeof(TYPE) * num_vals);
  assert(err == 0 && "Failed to allocate memory!");
  err = posix_memalign(
      (void**)&store_loc, CACHELINE_SIZE, sizeof(TYPE) * num_vals);
  assert(err == 0 && "Failed to allocate memory!");
  for (int i = 0; i < num_vals; i++) {
    store_vals[i] = i;
    store_loc[i] = -1;
  }

#ifdef LLVM_TRACE
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

  free(store_vals);
  free(store_loc);
  return 0;
}
