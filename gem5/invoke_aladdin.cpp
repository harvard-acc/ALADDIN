/* Sample program to show how to invoke Aladdin from simulated program.
 *
 * Compile this with -m32 -static to run this in gem5 syscall emulation mode,
 * then add -c <path to binary> in the command line command.
 *
 * Author: Sam Xi
 */

#include <stdio.h>
#include <stdint.h>

#include "aladdin_ioctl.h"
#include "aladdin_ioctl_req.h"

#define NOT_COMPLETED 0x000decaf

int main() {
  using namespace ALADDIN;

  /* Create a integer to store a sentinel value. You can initialize this to
   * anything you want.
   */
  int finish_flag = NOT_COMPLETED;
  printf("Finish flag has address %p.\n", (void*) &finish_flag);

  /* Invoke the accelerator with the special file descriptor, the benchmark
   * request code, and a pointer to the sentinel.
   */
  uint8_t k[32];
  printf("Mapping an array k at address 0x%x.\n", &(k[0]));
  mapArrayToAccelerator(MACHSUITE_AES_AES, "k", (void*) &(k[0]), 32);
  printf("About to invoke the accelerator.\n");
  invokeAccelerator(MACHSUITE_AES_AES, &finish_flag);
  printf("Accelerator invoked!\n");

  /* Spin until the finish_flag has changed. */
  while (finish_flag == NOT_COMPLETED) {}
  printf("Detected accelerator finish!\n");
  printf("Finish flag is now %d.\n", finish_flag);

  return 0;
}
