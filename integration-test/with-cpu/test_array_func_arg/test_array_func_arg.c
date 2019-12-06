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
void kernel(TYPE* array0_host,
            TYPE* array1_host,
            bool* selection_host,
            TYPE* array0_acc,
            TYPE* array1_acc,
            bool* selection_acc,
            size_t num_vals) {
  dmaLoad(array0_acc, array0_host, num_vals * sizeof(TYPE));
  dmaLoad(array1_acc, array1_host, num_vals * sizeof(TYPE));
  dmaLoad(selection_acc, selection_host, num_vals * sizeof(bool));

  loop: for (int i = 0; i < num_vals; i++) {
    if (selection_acc[i])
      child_func(array0_acc, i);
    else
      child_func(array1_acc, i);
  }

  dmaStore(array0_host, array0_acc, num_vals * sizeof(TYPE));
  dmaStore(array1_host, array1_acc, num_vals * sizeof(TYPE));
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
  TYPE* array0_host = NULL;
  TYPE* array1_host = NULL;
  bool* selection_host = NULL;
  TYPE* array0_acc = NULL;
  TYPE* array1_acc = NULL;
  bool* selection_acc = NULL;
  int err = posix_memalign(
      (void**)&array0_host, CACHELINE_SIZE, num_vals * sizeof(TYPE));
  err |= posix_memalign(
      (void**)&array1_host, CACHELINE_SIZE, num_vals * sizeof(TYPE));
  err |= posix_memalign(
      (void**)&selection_host, CACHELINE_SIZE, num_vals * sizeof(bool));
  err |= posix_memalign(
      (void**)&array0_acc, CACHELINE_SIZE, num_vals * sizeof(TYPE));
  err |= posix_memalign(
      (void**)&array1_acc, CACHELINE_SIZE, num_vals * sizeof(TYPE));
  err |= posix_memalign(
      (void**)&selection_acc, CACHELINE_SIZE, num_vals * sizeof(bool));
  if (err) {
    fprintf(stderr, "Unable to allocate aligned memory! Quitting test.\n");
    return -1;
  }

  for (int i = 0; i < num_vals; i++) {
    array0_host[i] = i;
    array1_host[i] = -i;
    selection_host[i] = (i % 2 == 0);
  }

#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      INTEGRATION_TEST, "array0_host", array0_host, num_vals * sizeof(TYPE));
  mapArrayToAccelerator(
      INTEGRATION_TEST, "array1_host", array1_host, num_vals * sizeof(TYPE));
  mapArrayToAccelerator(INTEGRATION_TEST, "selection_host", selection_host,
                        num_vals * sizeof(bool));

  fprintf(stdout, "Invoking accelerator!\n");
  invokeAcceleratorAndBlock(INTEGRATION_TEST);
  fprintf(stdout, "Accelerator finished!\n");
#else
  kernel(array0_host, array1_host, selection_host, array0_acc, array1_acc,
         selection_acc, num_vals);
#endif

  int num_failures =
      validate(array0_host, array1_host, selection_host, num_vals);
  if (num_failures != 0) {
    fprintf(stdout, "Test failed with %d errors.", num_failures);
    return -1;
  }
  fprintf(stdout, "Test passed!\n");

  free(array0_host);
  free(array1_host);
  free(selection_host);
  free(array0_acc);
  free(array1_acc);
  free(selection_acc);

  return 0;
}
