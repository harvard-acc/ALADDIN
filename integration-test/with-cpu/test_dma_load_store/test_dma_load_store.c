/* Test stores performed in the kernel.
 *
 * The values stored should be able to be loaded by the CPU after the kernel is
 * finished.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "gem5_harness.h"

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
void store_kernel(TYPE* store_vals_host,
                  TYPE* store_loc_host,
                  TYPE* store_vals_acc,
                  TYPE* store_loc_acc,
                  int num_vals) {
  dmaLoad(store_vals_acc, store_vals_host, num_vals * sizeof(TYPE));
  dmaLoad(store_loc_acc, store_loc_host, num_vals * sizeof(TYPE));

  loop: for (int i = 0; i < num_vals; i++)
    store_loc_acc[i] = store_vals_acc[i];

  dmaStore(store_loc_host, store_loc_acc, num_vals * sizeof(TYPE));
}

int main() {
  const int num_vals = 1024;
  TYPE* store_vals_host = NULL;
  TYPE* store_loc_host = NULL;
  TYPE* store_vals_acc = NULL;
  TYPE* store_loc_acc = NULL;
  int err = posix_memalign(
      (void**)&store_vals_host, CACHELINE_SIZE, sizeof(TYPE) * num_vals);
  err |= posix_memalign(
      (void**)&store_loc_host, CACHELINE_SIZE, sizeof(TYPE) * num_vals);
  err |= posix_memalign(
      (void**)&store_vals_acc, CACHELINE_SIZE, sizeof(TYPE) * num_vals);
  err |= posix_memalign(
      (void**)&store_loc_acc, CACHELINE_SIZE, sizeof(TYPE) * num_vals);
  assert(err == 0 && "Failed to allocate memory!");
  for (int i = 0; i < num_vals; i++) {
    store_vals_host[i] = i;
    store_loc_host[i] = -1;
  }

#ifdef GEM5_HARNESS
  mapArrayToAccelerator(INTEGRATION_TEST, "store_vals_host",
                        &(store_vals_host[0]), num_vals * sizeof(int));
  mapArrayToAccelerator(INTEGRATION_TEST, "store_loc_host",
                        &(store_loc_host[0]), num_vals * sizeof(int));

  fprintf(stdout, "Invoking accelerator!\n");
  invokeAcceleratorAndBlock(INTEGRATION_TEST);
  fprintf(stdout, "Accelerator finished!\n");
#else
  store_kernel(
      store_vals_host, store_loc_host, store_vals_acc, store_loc_acc, num_vals);
#endif

  int num_failures = test_stores(store_vals_host, store_loc_host, num_vals);
  if (num_failures != 0) {
    fprintf(stdout, "Test failed with %d errors.", num_failures);
    return -1;
  }
  fprintf(stdout, "Test passed!\n");

  free(store_vals_host);
  free(store_loc_host);
  free(store_vals_acc);
  free(store_loc_acc);
  return 0;

}
