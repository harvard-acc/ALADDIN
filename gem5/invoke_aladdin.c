/* Sample program to show how to invoke Aladdin from simulated program.
 *
 * Compile this with -m32 -static to run this in gem5 syscall emulation mode,
 * then add -c <path to binary> in the command line command.
 *
 * Author: Sam Xi
 */

#include <stdio.h>
#include <stdint.h>

#include "aladdin_sys_connection.h"
#include "aladdin_sys_constants.h"

int main() {
  /* Example of how to map an array named "k" that exists in the dynamic trace
   * to the simulator's address space, so that both the virtual addresses in the
   * trace and those used by the CPU will translate to the same physical
   * address.
   */
  uint8_t k[32];
  printf("Mapping an array k at address 0x%x.\n", &(k[0]));
  mapArrayToAccelerator(MACHSUITE_AES_AES, "k", (void*) &(k[0]), 32);

  /* Invoke the accelerator. This call will spin until the accelerator finishes.*/
  printf("Invoking the accelerator.\n");
  invokeAcceleratorAndBlock(MACHSUITE_AES_AES);

  printf("Detected accelerator finish!\n");

  return 0;
}
