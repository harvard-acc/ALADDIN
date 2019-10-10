// Test multiple accelerators running the same kernel.
//
// This test splits the DMA load and store task into a number of equal sized
// tiles, which will run on multiple accelerator.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "aladdin_sys_connection.h"
#include "aladdin_sys_constants.h"
#define TYPE int

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

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
  const int num_tiles = 16;
  TYPE* host_store_vals = NULL;
  TYPE* host_store_loc = NULL;
  TYPE* store_vals = NULL;
  TYPE* store_loc = NULL;
  int err = posix_memalign(
      (void**)&host_store_vals, CACHELINE_SIZE, sizeof(TYPE) * num_vals);
  err &= posix_memalign(
      (void**)&host_store_loc, CACHELINE_SIZE, sizeof(TYPE) * num_vals);
  err &= posix_memalign(
      (void**)&store_vals, CACHELINE_SIZE, sizeof(TYPE) * num_vals / num_tiles);
  err &= posix_memalign(
      (void**)&store_loc, CACHELINE_SIZE, sizeof(TYPE) * num_vals / num_tiles);
  assert(err == 0 && "Failed to allocate memory!");
  for (int i = 0; i < num_vals; i++) {
    host_store_vals[i] = i;
    host_store_loc[i] = -1;
  }

  int curr_accel_idx = 0;
  volatile int* finish_flags[MAX_NUM_ACCELS];
  // Run the tiles on multiple accelerators in parallel.
  for (int t = 0; t < num_tiles; t++) {
#ifdef GEM5_HARNESS
    mapArrayToAccelerator(INTEGRATION_TEST + curr_accel_idx, "host_store_vals",
                          &host_store_vals[num_vals / num_tiles * t],
                          num_vals / num_tiles * sizeof(TYPE));
    mapArrayToAccelerator(INTEGRATION_TEST + curr_accel_idx, "host_store_loc",
                          &host_store_loc[num_vals / num_tiles * t],
                          num_vals / num_tiles * sizeof(TYPE));
    setArrayMemoryType(
        INTEGRATION_TEST + curr_accel_idx, "host_store_vals", dma);
    setArrayMemoryType(
        INTEGRATION_TEST + curr_accel_idx, "host_store_loc", dma);
    fprintf(stdout, "Invoking accelerator!\n");
    finish_flags[curr_accel_idx] =
        invokeAcceleratorAndReturn(INTEGRATION_TEST + curr_accel_idx);
    if (++curr_accel_idx == num_accels) {
      // We have invoked all the accelerators we have, so we must wait until
      // they finish. For simplicity, we wait until all the accelerators
      // finished.
      for (int i = 0; i < num_accels; i++) {
        while (*finish_flags[i] == NOT_COMPLETED);
        fprintf(stdout, "Accelerator %d finished!\n", i);
        free((void*)finish_flags[i]);
      }
      curr_accel_idx = 0;
    }
#else
    // Trace mode goes into this path.
    llvmtracer_set_trace_name(get_trace_name(curr_accel_idx));
    store_kernel(&host_store_vals[num_vals / num_tiles * t],
                 &host_store_loc[num_vals / num_tiles * t], store_vals,
                 store_loc, num_vals / num_tiles);
    if (++curr_accel_idx == num_accels) {
      // If we run out of accelerators, start over from the first one.
      curr_accel_idx = 0;
    }
#endif
  }

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
