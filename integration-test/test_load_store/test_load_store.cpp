/* Test stores performed in the kernel.
 *
 * The values stored should be able to be loaded by the CPU after the kernel is
 * finished.
 */

#include <iostream>

#include "aladdin_sys_connection.h"
#include "aladdin_sys_constants.h"

int test_stores(int* store_vals, int* store_loc, int num_vals) {
  int num_failures = 0;
  for (int i = 0; i < num_vals; i++) {
    if (store_loc[i] != store_vals[i]) {
      std::cout << "FAILED: store_loc[" << i << "] = " << store_loc[i]
                << ", should be " << store_vals[i] << "\n";
      num_failures++;
    }
  }
  std::cout << std::endl;
  return num_failures;
}

// Read values from store_vals and copy them into store_loc.
void store_kernel(int* store_vals, int* store_loc, int num_vals) {
  for (int i = 0; i < num_vals; i++)
    store_loc[i] = store_vals[i];
}

int main() {

  const int num_vals = 2048;
  int* store_vals = new int[num_vals];
  int* store_loc = new int[num_vals];
  for (int i = 0; i < num_vals; i++) {
    store_vals[i] = i;
    store_loc[i] = -1;
  }

#ifdef LLVM_TRACE
  store_kernel(store_vals, store_loc, num_vals);
#elif ACCEL_SIMULATE_MODE
  mapArrayToAccelerator(
      INTEGRATION_TEST, "store_vals", &(store_vals[0]), num_vals * sizeof(int));
  mapArrayToAccelerator(
      INTEGRATION_TEST, "store_loc", &(store_loc[0]), num_vals * sizeof(int));

  int finish_flag = NOT_COMPLETED;
  invokeAccelerator(INTEGRATION_TEST, &finish_flag);
  std::cout << "Accelerator invoked!\n";
  while (finish_flag == NOT_COMPLETED) {
  }
  std::cout << "Accelerator finished!\n";
  std::cout << "Finish flag value is " << std::hex << finish_flag << std::dec
            << "\n";
#endif

  int num_failures = test_stores(store_vals, store_loc, num_vals);
  if (num_failures == 0)
    std::cout << "Test passed!" << std::endl;
  else
    std::cout << "Test failed with " << num_failures << " errors." << std::endl;

  return 0;
}
