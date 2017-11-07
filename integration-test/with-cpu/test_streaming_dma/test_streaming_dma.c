/* Test streaming DMA operations.
 *
 * The values stored should be able to be loaded by the CPU after the kernel is
 * finished.
 */

#include <stdio.h>
#include "aladdin_sys_connection.h"
#include "aladdin_sys_constants.h"
#define TYPE int

#define CACHELINE_SIZE 64

#include "dma_interface.h"

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


// local_values and local_target are pointers to accelerator-local scratchpads.
// host_values and host_target are pointers to memory on the host CPU.
// IMPORTANT: These addresses are DIFFERENT values.
void store_kernel(TYPE* host_values,
                  TYPE* host_target,
                  TYPE* local_values,
                  TYPE* local_target,
                  int num_vals) {
  setReadyBits(local_values, (num_vals / 2) * sizeof(TYPE), 0);
  dmaLoad(local_values, host_values, (num_vals / 2) * sizeof(TYPE));
  dmaFence();

  loop0: for (int i = 0; i < num_vals/2; i++)
    local_target[i] = local_values[i];

  // Store the first iteration's results while loading in data for the next.
  dmaStore(host_target, local_target, (num_vals/2) * sizeof(TYPE));
  setReadyBits(&local_values[num_vals/2], (num_vals/2) * sizeof(TYPE), 0);
  dmaLoad(&local_values[num_vals/2], &host_values[num_vals/2],
          (num_vals / 2) * sizeof(TYPE));

  loop1: for (int i = num_vals/2; i < num_vals; i++)
    local_target[i] = local_values[i];

  dmaStore(&host_target[num_vals/2], &local_target[num_vals/2],
           (num_vals/2) * sizeof(TYPE));
}

int main() {
  const int num_vals = 1024;
  TYPE *accel_vals = NULL, *accel_target = NULL, *host_vals = NULL,
       *host_target = NULL;
  int err = posix_memalign(
      (void**)&accel_vals, CACHELINE_SIZE, num_vals * sizeof(TYPE));
  err |= posix_memalign(
      (void**)&accel_target, CACHELINE_SIZE, num_vals * sizeof(TYPE));
  err |= posix_memalign(
      (void**)&host_vals, CACHELINE_SIZE, num_vals * sizeof(TYPE));
  err |= posix_memalign(
      (void**)&host_target, CACHELINE_SIZE, num_vals * sizeof(TYPE));
  if (err) {
    fprintf(stderr, "Unable to allocate aligned memory!\n");
    return -1;
  }

  for (int i = 0; i < num_vals; i++) {
    accel_vals[i] = -1;
    accel_target[i] = -1;
    host_vals[i] = i;
    host_target[i] = -1;
  }

#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      INTEGRATION_TEST, "host_values", host_vals, num_vals * sizeof(int));
  mapArrayToAccelerator(
      INTEGRATION_TEST, "host_target", host_target, num_vals * sizeof(int));

  fprintf(stdout, "Invoking accelerator!\n");
  invokeAcceleratorAndBlock(INTEGRATION_TEST);
  fprintf(stdout, "Accelerator finished!\n");
#else
  store_kernel(host_vals, host_target, accel_vals, accel_target, num_vals);
#endif

  int num_failures = test_stores(host_vals, host_target, num_vals);
  if (num_failures != 0) {
    fprintf(stdout, "Test failed with %d errors\n.", num_failures);
    return -1;
  }
  fprintf(stdout, "Test passed!\n");
  free(accel_vals);
  free(accel_target);
  free(host_vals);
  free(host_target);
  return 0;
}
