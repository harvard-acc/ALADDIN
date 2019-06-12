#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "aladdin_sys_connection.h"
#include "aladdin_sys_constants.h"
#include "gem5/sampling_interface.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

#define CACHELINE_SIZE 32

void test_loop_sampling(int* inputs,
                        int* results,
                        int inputs_size,
                        int sample_size) {
#ifdef DMA_MODE
  dmaLoad(inputs, inputs, inputs_size * sizeof(int));
#endif
  setSamplingFactor("loop", inputs_size / sample_size);
  loop:
  for (int i = 0; i < sample_size; i++) {
    // Do some computation.
    results[i] = inputs[i] * inputs[i] * inputs[i];
    results[i] *= i;
  }

#ifdef DMA_MODE
  dmaStore(results, results, inputs_size * sizeof(int));
#endif
}

int main() {
  int inputs_size = 1024;
  int sample_size = 1;
  int *inputs, *results;
  int err = posix_memalign(
      (void**)&inputs, CACHELINE_SIZE, sizeof(int) * inputs_size);
  assert(err == 0 && "Failed to allocate memory!");
  err = posix_memalign(
      (void**)&results, CACHELINE_SIZE, sizeof(int) * inputs_size);
  assert(err == 0 && "Failed to allocate memory!");
  for (int i = 0; i < inputs_size; i++) {
    inputs[i] = i;
  }
#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      INTEGRATION_TEST, "inputs", &(inputs[0]), inputs_size * sizeof(int));
  mapArrayToAccelerator(
      INTEGRATION_TEST, "results", &(results[0]), inputs_size * sizeof(int));

  fprintf(stdout, "Invoking accelerator!\n");
  invokeAcceleratorAndBlock(INTEGRATION_TEST);
  fprintf(stdout, "Accelerator finished!\n");
#else
  test_loop_sampling(inputs, results, inputs_size, sample_size);
#endif
  return 0;
}
