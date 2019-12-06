/* Test stores performed in the kernel across multiple invocations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "gem5_harness.h"

#define TYPE int
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
void store_kernel(TYPE* store_vals_host,
                  TYPE* store_loc_host,
                  TYPE* store_vals_acc,
                  TYPE* store_loc_acc,
                  int starting_offset,
                  int num_vals) {
  dmaLoad(store_vals_acc, &store_vals_host[starting_offset],
          num_vals * sizeof(TYPE));
  dmaLoad(
      store_loc_acc, &store_loc_host[starting_offset], num_vals * sizeof(TYPE));

  loop: for (int i = 0; i < num_vals; i++)
    store_loc_acc[i] = store_vals_acc[i];

  dmaStore(
      &store_loc_host[starting_offset], store_loc_acc, num_vals * sizeof(TYPE));
}

int main() {
  const int num_vals = 1024;
  // Calling dmaLoad and dmaStore will cause memory to be overwritten in the
  // original arrays. Sometimes this is okay, sometimes it's not. To avoid this
  // problem, we allocate additional scratch space that the accelerator will
  // use, which will be the first temp_storage elements of each array.
  const int temp_storage = num_vals/2;
  TYPE* store_vals_host = NULL;
  TYPE* store_loc_host = NULL;
  TYPE* store_vals_acc = NULL;
  TYPE* store_loc_acc = NULL;
  int err = posix_memalign((void**)&store_vals_host, CACHELINE_SIZE,
                           sizeof(TYPE) * (num_vals + temp_storage));
  err |= posix_memalign((void**)&store_loc_host, CACHELINE_SIZE,
                        sizeof(TYPE) * (num_vals + temp_storage));
  err |= posix_memalign((void**)&store_vals_acc, CACHELINE_SIZE,
                        sizeof(TYPE) * (num_vals + temp_storage));
  err |= posix_memalign((void**)&store_loc_acc, CACHELINE_SIZE,
                        sizeof(TYPE) * (num_vals + temp_storage));
  assert(err == 0 && "Failed to allocate memory!");
  for (int i = temp_storage; i < temp_storage + num_vals; i++) {
    store_vals_host[i] = i;
    store_loc_host[i] = -1;
  }

#ifdef GEM5_HARNESS
  // Arrays only have to mapped once.
  mapArrayToAccelerator(INTEGRATION_TEST, "store_vals_host", store_vals_host,
                        (num_vals + temp_storage) * sizeof(int));
  mapArrayToAccelerator(INTEGRATION_TEST, "store_loc_host", store_loc_host,
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
  store_kernel(store_vals_host, store_loc_host, store_vals_acc, store_loc_acc,
               temp_storage, temp_storage);
  store_kernel(store_vals_host, store_loc_host, store_vals_acc, store_loc_acc,
               2 * temp_storage, temp_storage);
#endif

  int num_failures =
      test_stores(store_vals_host, store_loc_host, num_vals, temp_storage);

  if (num_failures != 0) {
    fprintf(stdout, "Test failed with %d errors.\n", num_failures);
    return -1;
  }
  fprintf(stdout, "Test passed!\n");

  free(store_vals_host);
  free(store_loc_host);
  free(store_vals_acc);
  free(store_loc_acc);
  return 0;
}
