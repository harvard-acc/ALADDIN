// Test multiple accelerators running the same kernel.
//
// This test splits the DMA load and store task into a number of equal sized
// tiles, which will run on multiple accelerator.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "gem5_harness.h"

#define TYPE int
#define CACHELINE_SIZE 64
#define MAX_NUM_ACCELS 4

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
  hostLoad(store_vals_acc, store_vals_host, num_vals * sizeof(TYPE));
  hostLoad(store_loc_acc, store_loc_host, num_vals * sizeof(TYPE));

  loop: for (int i = 0; i < num_vals; i++)
    store_loc_acc[i] = store_vals_acc[i];

  hostStore(store_loc_host, store_loc_acc, num_vals * sizeof(TYPE));
}

char* get_trace_name(int accel_idx) {
  char* buffer = (char*)malloc(30);
  snprintf(buffer, 30, "dynamic_trace_acc%d.gz", accel_idx);
  return buffer;
}

int main() {
  // Number of accelerators.
  const int num_accels = 4;
  assert(num_accels <= MAX_NUM_ACCELS &&
         "Out of the maximum number of accelerators! Add more accelerator in "
         "gem5.cfg if you have more.");
  const int num_vals = 1024;
  // Number of tiles to split the task. It needs to divide num_vals. When you
  // change the number of tiles, remember to also change the size of scratchpads
  // (i.e., store_vals, store_loc) in test_multiple_accelerators.cfg.
  const int num_tiles = 8;
  TYPE* store_vals_host = NULL;
  TYPE* store_loc_host = NULL;
  TYPE* store_vals_acc = NULL;
  TYPE* store_loc_acc = NULL;
  int err = posix_memalign(
      (void**)&store_vals_host, CACHELINE_SIZE, sizeof(TYPE) * num_vals);
  err &= posix_memalign(
      (void**)&store_loc_host, CACHELINE_SIZE, sizeof(TYPE) * num_vals);
  err &= posix_memalign((void**)&store_vals_acc, CACHELINE_SIZE,
                        sizeof(TYPE) * num_vals / num_tiles);
  err &= posix_memalign((void**)&store_loc_acc, CACHELINE_SIZE,
                        sizeof(TYPE) * num_vals / num_tiles);
  assert(err == 0 && "Failed to allocate memory!");
  for (int i = 0; i < num_vals; i++) {
    store_vals_host[i] = i;
    store_loc_host[i] = -1;
  }

  int curr_accel_idx = 0;
  volatile int* finish_flags[MAX_NUM_ACCELS];
  // Run the tiles on multiple accelerators in parallel.
  for (int t = 0; t < num_tiles; t++) {
#ifdef GEM5_HARNESS
    mapArrayToAccelerator(INTEGRATION_TEST + curr_accel_idx, "store_vals_host",
                          &store_vals_host[num_vals / num_tiles * t],
                          num_vals / num_tiles * sizeof(TYPE));
    mapArrayToAccelerator(INTEGRATION_TEST + curr_accel_idx, "store_loc_host",
                          &store_loc_host[num_vals / num_tiles * t],
                          num_vals / num_tiles * sizeof(TYPE));
    setArrayMemoryType(
        INTEGRATION_TEST + curr_accel_idx, "store_vals_host", dma);
    setArrayMemoryType(
        INTEGRATION_TEST + curr_accel_idx, "store_loc_host", dma);
    fprintf(stdout, "Invoking accelerator!\n");
    finish_flags[curr_accel_idx] =
        invokeAcceleratorAndReturn(INTEGRATION_TEST + curr_accel_idx);
    if (++curr_accel_idx == num_accels) {
      // We have invoked all the accelerators we have, so we must wait until
      // they finish. For simplicity, we wait until all the accelerators
      // finished.
      for (int i = 0; i < num_accels; i++) {
        waitForAccelerator(finish_flags[i]);
        fprintf(stdout, "Accelerator %d finished!\n", i);
        free((void*)finish_flags[i]);
      }
      curr_accel_idx = 0;
    }
#else
    // Trace mode goes into this path.
    llvmtracer_set_trace_name(get_trace_name(curr_accel_idx));
    store_kernel(&store_vals_host[num_vals / num_tiles * t],
                 &store_loc_host[num_vals / num_tiles * t], store_vals_acc,
                 store_loc_acc, num_vals / num_tiles);
    if (++curr_accel_idx == num_accels) {
      // If we run out of accelerators, start over from the first one.
      curr_accel_idx = 0;
    }
#endif
  }

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
