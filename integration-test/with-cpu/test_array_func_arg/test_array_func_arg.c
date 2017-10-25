/* Test stores performed in the kernel.
 *
 * The values stored should be able to be loaded by the CPU after the kernel is
 * finished.
 *
 * Performance is dependent on the relationship between the unrolling factor of
 * the inner loop and cache queue size/bandwidth.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "gem5_harness.h"

#define TYPE int
#define CACHELINE_SIZE 32

int validate(TYPE* array0, TYPE* array1, bool* selection, int num_vals) {
  int num_failures = 0;
  for (int i = 0; i < num_vals; i++) {
    if (selection[i]) {
      if (array0[i] != -i) {
        fprintf(stdout, "FAILED: array0[%d] = %d, should be %d\n", i, array0[i],
                -i);
        num_failures++;
      }
    } else {
      if (array1[i] != i) {
        fprintf(
            stdout, "FAILED: array1[%d] = %d, should be %d\n", i, array1[i], i);
        num_failures++;
      }
    }
  }
  return num_failures;
}


void child_func(TYPE* array, size_t idx) {
  array[idx] = -array[idx];
}

// Read values from store_vals and copy them into store_loc.
void kernel(TYPE* array0, TYPE* array1, bool* selection, size_t num_vals) {
#ifdef DMA_MODE
  dmaLoad(array0, array0, num_vals * sizeof(TYPE));
  dmaLoad(array1, array1, num_vals * sizeof(TYPE));
  dmaLoad(selection, selection, num_vals * sizeof(bool));
#endif

  loop: for (int i = 0; i < num_vals; i++) {
    if (selection[i])
      child_func(array0, i);
    else
      child_func(array1, i);
  }

#ifdef DMA_MODE
  dmaStore(array0, array0, num_vals * sizeof(TYPE));
  dmaStore(array1, array1, num_vals * sizeof(TYPE));
#endif
}

void print_guard(TYPE* array, size_t len, size_t guard_size) {
  for (size_t i = 0; i < guard_size; i++) {
    printf("%x ", array[i]);
    if ((i+1) % 4 == 0)
      printf("\n");
  }
  for (size_t i = len - guard_size; i < len; i++) {
    printf("%x ", array[i]);
    if ((i+1) % 4 == 0)
      printf("\n");
  }
  printf("\n");
}

int main() {
  const size_t num_vals = 128;
  TYPE* array0;
  TYPE* array1;
  bool* selection;
  posix_memalign((void**)&array0, CACHELINE_SIZE, num_vals * sizeof(TYPE));
  posix_memalign((void**)&array1, CACHELINE_SIZE, num_vals * sizeof(TYPE));
  posix_memalign((void**)&selection, CACHELINE_SIZE, num_vals * sizeof(bool));

  for (int i = 0; i < num_vals; i++) {
    array0[i] = i;
    array1[i] = -i;
    selection[i] = (i % 2 == 0);
  }

#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      INTEGRATION_TEST, "array0", array0, num_vals * sizeof(TYPE));
  mapArrayToAccelerator(
      INTEGRATION_TEST, "array1", array1, num_vals * sizeof(TYPE));
  mapArrayToAccelerator(
      INTEGRATION_TEST, "selection", selection, num_vals * sizeof(bool));

  fprintf(stdout, "Invoking accelerator!\n");
  invokeAcceleratorAndBlock(INTEGRATION_TEST);
  fprintf(stdout, "Accelerator finished!\n");
#else
  kernel(array0, array1, selection, num_vals);
#endif

  int num_failures = validate(array0, array1, selection, num_vals);
  if (num_failures != 0) {
    fprintf(stdout, "Test failed with %d errors.", num_failures);
    return -1;
  }
  fprintf(stdout, "Test passed!\n");

  free(array0);
  free(array1);
  free(selection);

  return 0;
}
