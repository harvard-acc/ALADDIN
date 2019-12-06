#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "gem5_harness.h"

#define CACHELINE_SIZE 32

void test_loop_sampling(int* inputs_host,
                        int* results_host,
                        int* inputs_acc,
                        int* results_acc,
                        int inputs_size,
                        int sample_size) {
  dmaLoad(inputs_acc, inputs_host, inputs_size * sizeof(int));
  float sampling_factor = inputs_size * 1.0 / sample_size;
  setSamplingFactor("loop0", sampling_factor);
  setSamplingFactor("loop1", sampling_factor);
  setSamplingFactor("loop2", sampling_factor);
  setSamplingFactor("loop3", 1);
  setSamplingFactor("loop4", sampling_factor);
  loop0:
  for (int k = 0; k < sample_size; k++) {
    results_acc[k] = inputs_acc[k] * 2;
    loop1:
    for (int i = 0; i < sample_size; i++) {
      // Do some computation.
      results_acc[i] = inputs_acc[i] * inputs_acc[i] * inputs_acc[i];
      results_acc[i] *= i;
      loop2:
      for (int j = 0; j < sample_size; j++) {
        results_acc[i] += results_acc[j];
      }
      loop3:
      for (int j = 0; j < inputs_size; j++) {
        results_acc[i] *= inputs_acc[j];
      }
    }
  }
  loop4:
  for (int i = 0; i < sample_size; i++) {
    results_acc[i] -= inputs_acc[i];
  }

  dmaStore(results_host, results_acc, inputs_size * sizeof(int));
}

int main() {
  int inputs_size = 32;
  int sample_size = 1;
  int* inputs_host = NULL;
  int* results_host = NULL;
  int* inputs_acc = NULL;
  int* results_acc = NULL;
  int err = posix_memalign(
      (void**)&inputs_host, CACHELINE_SIZE, sizeof(int) * inputs_size);
  err |= posix_memalign(
      (void**)&results_host, CACHELINE_SIZE, sizeof(int) * inputs_size);
  err |= posix_memalign(
      (void**)&inputs_acc, CACHELINE_SIZE, sizeof(int) * inputs_size);
  err |= posix_memalign(
      (void**)&results_acc, CACHELINE_SIZE, sizeof(int) * inputs_size);
  assert(err == 0 && "Failed to allocate memory!");
  for (int i = 0; i < inputs_size; i++) {
    inputs_host[i] = i;
  }
#ifdef GEM5_HARNESS
  mapArrayToAccelerator(INTEGRATION_TEST,
                        "inputs_host",
                        &(inputs_host[0]),
                        inputs_size * sizeof(int));
  mapArrayToAccelerator(INTEGRATION_TEST,
                        "results_host",
                        &(results_host[0]),
                        inputs_size * sizeof(int));

  fprintf(stdout, "Invoking accelerator!\n");
  invokeAcceleratorAndBlock(INTEGRATION_TEST);
  fprintf(stdout, "Accelerator finished!\n");
#else
  test_loop_sampling(inputs_host,
                     results_host,
                     inputs_acc,
                     results_acc,
                     inputs_size,
                     sample_size);
#endif

  free(inputs_host);
  free(results_host);
  free(inputs_acc);
  free(results_acc);
  return 0;
}
