/* Test stores performed in the kernel.
 *
 * The values stored should be able to be loaded by the CPU after the kernel is
 * finished.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "aladdin_sys_connection.h"
#include "aladdin_sys_constants.h"
#define TYPE int

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

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
void store_kernel(TYPE* host_store_vals,
                  TYPE* host_store_loc,
                  TYPE* store_vals,
                  TYPE* store_loc,
                  int num_vals) {
#ifdef DMA_MODE
  hostLoad(store_vals, host_store_vals, num_vals * sizeof(TYPE));
  hostLoad(store_loc, host_store_loc, num_vals * sizeof(TYPE));
#endif

  loop: for (int i = 0; i < num_vals; i++)
    store_loc[i] = store_vals[i];

#ifdef DMA_MODE
  hostStore(host_store_loc, store_loc, num_vals * sizeof(TYPE));
#endif
}

int main() {
  const int num_vals = 1024;
  TYPE* host_store_vals = NULL;
  TYPE* host_store_loc = NULL;
  TYPE* store_vals = NULL;
  TYPE* store_loc = NULL;
  int err = posix_memalign(
      (void**)&host_store_vals, CACHELINE_SIZE, sizeof(TYPE) * num_vals);
  err &= posix_memalign(
      (void**)&host_store_loc, CACHELINE_SIZE, sizeof(TYPE) * num_vals);
  err &= posix_memalign(
      (void**)&store_vals, CACHELINE_SIZE, sizeof(TYPE) * num_vals / 2);
  err &= posix_memalign(
      (void**)&store_loc, CACHELINE_SIZE, sizeof(TYPE) * num_vals / 2);
  assert(err == 0 && "Failed to allocate memory!");
  for (int i = 0; i < num_vals; i++) {
    host_store_vals[i] = i;
    host_store_loc[i] = -1;
  }

#ifdef GEM5_HARNESS
  mapArrayToAccelerator(INTEGRATION_TEST, "host_store_vals",
                        &(host_store_vals[0]), num_vals / 2 * sizeof(TYPE));
  mapArrayToAccelerator(INTEGRATION_TEST, "host_store_loc",
                        &(host_store_loc[0]), num_vals / 2 * sizeof(TYPE));
  // Use DMA for the first half of the data.
  setArrayMemoryType(INTEGRATION_TEST, "host_store_vals", dma);
  setArrayMemoryType(INTEGRATION_TEST, "host_store_loc", dma);
  fprintf(stdout, "Invoking accelerator!\n");
  volatile int* finish_flag0 = invokeAcceleratorAndReturn(INTEGRATION_TEST);

  mapArrayToAccelerator(INTEGRATION_TEST, "host_store_vals",
                        &(host_store_vals[num_vals / 2]),
                        num_vals / 2 * sizeof(TYPE));
  mapArrayToAccelerator(INTEGRATION_TEST, "host_store_loc",
                        &(host_store_loc[num_vals / 2]),
                        num_vals / 2 * sizeof(TYPE));
  // Use ACP for the second half of the data.
  setArrayMemoryType(INTEGRATION_TEST, "host_store_vals", acp);
  setArrayMemoryType(INTEGRATION_TEST, "host_store_loc", acp);
  volatile int* finish_flag1 = invokeAcceleratorAndReturn(INTEGRATION_TEST);

  // Wait the two invocations to finish.
  while (*finish_flag0 == NOT_COMPLETED || *finish_flag1 == NOT_COMPLETED)
    ;
  fprintf(stdout, "Accelerator finished!\n");

  free((void*)finish_flag0);
  free((void*)finish_flag1);
#else
  store_kernel(
      host_store_vals, host_store_loc, store_vals, store_loc, num_vals / 2);
  store_kernel(&host_store_vals[num_vals / 2], &host_store_loc[num_vals / 2],
               store_vals, store_loc, num_vals / 2);
#endif

  int num_failures = test_stores(host_store_vals, host_store_loc, num_vals);
  if (num_failures != 0) {
    fprintf(stdout, "Test failed with %d errors.", num_failures);
    return -1;
  }
  fprintf(stdout, "Test passed!\n");

  free(host_store_vals);
  free(host_store_loc);
  free(store_vals);
  free(store_loc);
  return 0;
}
