/* Test stores performed in the kernel across multiple invocations.
 */

#include <stdio.h>
#include "aladdin_sys_connection.h"
#include "aladdin_sys_constants.h"
#define TYPE int

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

#define CACHELINE_SIZE 32

int test_stores(TYPE* store_vals, TYPE* store_loc,
                int num_vals,
                int temp_storage) {
  int num_failures = 0;
  for (int i = temp_storage; i < temp_storage + num_vals; i++) {
    if (store_loc[i] != store_vals[i]) {
      fprintf(stdout, "FAILED: store_loc[%d] = %d, should be %d\n",
                               i, store_loc[i], store_vals[i]);
      num_failures++;
    }
  }
  return num_failures;
}


// Read values from store_vals and copy them into store_loc.
void store_kernel(TYPE* store_vals,
                  TYPE* store_loc,
                  int starting_offset,
                  int num_vals) {
#ifdef DMA_MODE
  dmaLoad(store_vals, &store_vals[starting_offset], num_vals * sizeof(TYPE));
  dmaLoad(store_loc, &store_loc[starting_offset], num_vals * sizeof(TYPE));
#endif

  loop: for (int i = 0; i < num_vals; i++)
    store_loc[i] = store_vals[i];

#ifdef DMA_MODE
  dmaStore(&store_loc[starting_offset], store_loc, num_vals * sizeof(TYPE));
#endif
}

int main() {
  const int num_vals = 1024;
  // Calling dmaLoad and dmaStore will cause memory to be overwritten in the
  // original arrays. Sometimes this is okay, sometimes it's not. To avoid this
  // problem, we allocate additional scratch space that the accelerator will
  // use, which will be the first temp_storage elements of each array.
  const int temp_storage = num_vals/2;
  TYPE* store_vals;
  TYPE* store_loc;
  posix_memalign((void**)&store_vals, CACHELINE_SIZE,
                 sizeof(TYPE) * (num_vals + temp_storage));
  posix_memalign((void**)&store_loc, CACHELINE_SIZE,
                 sizeof(TYPE) * (num_vals + temp_storage));
  for (int i = temp_storage; i < temp_storage + num_vals; i++) {
    store_vals[i] = i;
    store_loc[i] = -1;
  }

#ifdef GEM5_HARNESS
  // Arrays only have to mapped once.
  mapArrayToAccelerator(INTEGRATION_TEST, "store_vals", store_vals,
                        (num_vals + temp_storage) * sizeof(int));
  mapArrayToAccelerator(INTEGRATION_TEST, "store_loc", store_loc,
                        (num_vals + temp_storage) * sizeof(int));

  // Accelerator will be invoked twice, but there's no need to specify the new
  // starting offset because it's already in the trace.
  fprintf(stdout, "Invoking accelerator!\n");
  invokeAcceleratorAndBlock(INTEGRATION_TEST);
  fprintf(stdout, "Accelerator finished!\n");

  fprintf(stdout, "Invoking accelerator!\n");
  invokeAcceleratorAndBlock(INTEGRATION_TEST);
  fprintf(stdout, "Accelerator finished!\n");
#else
  store_kernel(store_vals, store_loc, temp_storage, temp_storage);
  store_kernel(store_vals, store_loc, 2 * temp_storage, temp_storage);
#endif

  int num_failures = test_stores(store_vals, store_loc, num_vals, temp_storage);

  free(store_vals);
  free(store_loc);

  if (num_failures != 0) {
    fprintf(stdout, "Test failed with %d errors.\n", num_failures);
    return -1;
  }
  fprintf(stdout, "Test passed!\n");
  return 0;
}
