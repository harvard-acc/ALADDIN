#ifndef __ALADDIN_SYS_CONNECTION_H__
#define __ALADDIN_SYS_CONNECTION_H__

#include <stdbool.h>
#include <stdlib.h>

/* Support for invoking accelerators via ioctl() system call. A user level
 * program invokes an accelerator in the system like so:
 *
 *   ioctl(ALADDIN_FD, REQUEST_CODE, finish_flag);
 *
 * ALADDIN_FD is a special file descriptor reserved for Aladdin.
 * The request code specifies which accelerator to activate. This requires
 * agreement with the accelerator_id parameter used in the Python scripts to
 * bring up the system.
 * The finish flag is a shared memory address that the accelerator will write to
 * when it is finished. This will signal to the core that the accelerator is
 * finished.
 */

// Parameters for constructing the address translation in gem5.
typedef struct _aladdin_map_t {
  // Name of the array as it appears in the trace.
  const char* array_name;
  // Pointer to the array.
  void* addr;
  // Accelerator ioctl request code.
  unsigned request_code;
  // Size of the array.
  size_t size;
} aladdin_map_t;

#ifdef __cplusplus
extern "C" {
#endif

// Checks of the request code is valid.
bool isValidRequestCode(unsigned req);

/* Schedules the accelerator for execution in gem5 and spin waits until the accelerator returns.
 *
 * Args:
 *   req_code: Accelerator request code.
 *
 * Returns:
 *   Nothing.
 */
void invokeAcceleratorAndBlock(unsigned req_code);

/* Schedules the accelerator for execution in gem5 and returns immediately. The
 * client code is respionsible for checking for accelerator completion and
 * deleting the finish_flag pointer.
 *
 * Args:
 *   req_code: Accelerator request code.
 *
 * Returns:
 *   A new pointer to a finish flag integer. The client code should delete this
 *     when it is finished with it.
 */
int* invokeAcceleratorAndReturn(unsigned req_code);

/* Trigger gem5 to dump its stats and then resume simulation.
 *
 * This is accomplished using a special request code in lieu of one assigned to
 * an accelerator.
 *
 * Args:
 *   stats_desc: A description of what the code just performed. This will be
 *   dumped inline with the stats.
 */
void dumpGem5Stats(char* stats_desc);

/* Trigger gem5 to reset all its stats and then resume simulation.
 *
 * In this case, statistics are NOT dumped to the stats file.
 */
void resetGem5Stats();

/* Create a virtual to physical address mapping for the specified array for
 * the dynamic trace used in Aladdin.
 *
 * The dynamic trace contains virtual addresses that are (usually) not the
 * same as the virtual addresses assigned by gem5. In order for Aladdin to
 * support coherence with the CPU, we need to create a new address
 * translation for the dynamic trace.
 *
 * Args:
 *   req: Request code for the accelerator.
 *   array_name: Name of the array or variable.
 *   addr: Pointer to the array in the simulated address space.
 *   size: Size of the array.
 *
 * Returns:
 *   Nothing.
 */
void mapArrayToAccelerator(
    unsigned req_code, const char* array_name, void* addr, size_t size);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
