#ifndef __ALADDIN_SYS_CONNECTION_H__
#define __ALADDIN_SYS_CONNECTION_H__

#include <stdbool.h>
#include <stdlib.h>

#include "../common/MemoryType.h"

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

// Parameters for setting an array's memory access type.
typedef struct _aladdin_mem_desc_t {
  // Name of the array as it appears in the trace.
  const char* array_name;
  // Memory access type.
  MemoryType mem_type;
  // Accelerator ioctl request code.
  unsigned request_code;
} aladdin_mem_desc_t;

// Struct for passing accelerator parameters in gem5.
typedef struct _aladdin_params_t {
  volatile int* finish_flag;
  // This is a pointer to a struct that is defined by the accelerator SIMULATOR
  // to pass custom parameters, not the USER. For example, the user who is using
  // Aladdin to model the accelerator cannot use this to store arbitrary
  // parameters. It is intended to be used by simulator developers.
  void* accel_params_ptr;
  // Actual size of the parameters struct defined by the accelerator simulator.
  size_t size;
} aladdin_params_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Helper function to set up the accelerator parameters. */
aladdin_params_t* getParams(volatile int* finish_flag,
                            int finish_flag_val,
                            void* accel_params_ptr,
                            int size);

// Suspend the CPU thread context *until* finish_flag is not NOT_COMPLETED.
void suspendCPUUntilFlagChanges(volatile int* finish_flag);

/* Schedules the accelerator for execution in gem5 and spin waits until the
 *accelerator returns.
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
volatile int* invokeAcceleratorAndReturn(unsigned req_code);

/* The only difference from invokeAcceleratorAndReturn is this one does not
 * allocate memory for the finish_flag pointer, and the client code is
 * responsible for that.
 */
void invokeAcceleratorAndReturn2(unsigned req_code, volatile int* finish_flag);

/* Wait until the accelerator sends a finish signal. This suspends the CPU
 * thread.
 *
 * Args:
 *   finish_flag: A flag allocated in the shared memory used for synchronization
 *       between the accelerator and the CPU.
 */
void waitForAccelerator(volatile int* finish_flag);

/* Trigger gem5 to dump its stats and then resume simulation.
 *
 * This is accomplished using a special request code in lieu of one assigned to
 * an accelerator.
 *
 * Args:
 *   stats_desc: A description of what the code just performed. This will be
 *   dumped inline with the stats.
 */
void dumpGem5Stats(const char* stats_desc);

/* Trigger gem5 to reset all its stats and then resume simulation.
 *
 * In this case, statistics are NOT dumped to the stats file.
 */
void resetGem5Stats();

/* Start reading the dynamic trace from the beginning. */
void resetTrace(unsigned req_code);

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
void mapArrayToAccelerator(unsigned req_code,
                           const char* array_name,
                           void* addr,
                           size_t size);

/* Set the memory access type for the specified array.
 *
 * This can be used to set (or change) the memory access type of an array.
 *
 * Args:
 *   req_code: Request code for the accelerator.
 *   array_name: Name of the array or variable.
 *   mem_type: String value of the memory type.
 */
void setArrayMemoryType(unsigned req_code,
                        const char* array_name,
                        MemoryType mem_type);


/* Return the string value of the memory type. */
const char* memoryTypeToString(MemoryType mem_type);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
