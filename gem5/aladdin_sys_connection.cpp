#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "aladdin_sys_connection.h"
#include "aladdin_sys_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

aladdin_params_t* getParams(volatile int* finish_flag,
                            int finish_flag_val,
                            void* accel_params_ptr,
                            int size) {
  aladdin_params_t* params =
      (aladdin_params_t*)malloc(sizeof(aladdin_params_t));
  params->finish_flag = finish_flag;
  if (params->finish_flag == NULL)
    params->finish_flag = (volatile int*)malloc(sizeof(int));
  *(params->finish_flag) = finish_flag_val;
  params->accel_params_ptr = accel_params_ptr;
  params->size = size;
  return params;
}

void suspendCPUUntilFlagChanges(volatile int* finish_flag) {
  // The loading of the finish flag's value, the comparison of that value with
  // the expected value, and the actual blocking of the CPU thread should happen
  // atomically, because a wakeup can happen in the interim leading to a
  // deadlock. The following ioctl with the WAIT_FINISH_SIGNAL request code
  // achieve this. It suspends the CPU context until it's woken up by the
  // accelerator. Upon a wakeup, to ward off spurious wakeups induced by other
  // system events (e.g., atomic memory accesses), we ensure the accelerator has
  // finished by checking the finish flag again.
  while (1) {
    ioctl(ALADDIN_FD, WAIT_FINISH_SIGNAL, finish_flag);
    if (*finish_flag != NOT_COMPLETED)
      break;
  }
}

void invokeAcceleratorAndBlock(unsigned req_code) {
  aladdin_params_t* params = getParams(NULL, NOT_COMPLETED, NULL, 0);
  ioctl(ALADDIN_FD, req_code, params);
  suspendCPUUntilFlagChanges(params->finish_flag);
  free((void*)(params->finish_flag));
  free(params);
}

volatile int* invokeAcceleratorAndReturn(unsigned req_code) {
  aladdin_params_t* params = getParams(NULL, NOT_COMPLETED, NULL, 0);
  ioctl(ALADDIN_FD, req_code, params);
  volatile int* finish_flag = params->finish_flag;
  free(params);
  return finish_flag;
}

void invokeAcceleratorAndReturn2(unsigned req_code, volatile int* finish_flag) {
  aladdin_params_t* params = getParams(finish_flag, NOT_COMPLETED, NULL, 0);
  ioctl(ALADDIN_FD, req_code, params);
  free(params);
}

void waitForAccelerator(volatile int* finish_flag) {
  suspendCPUUntilFlagChanges(finish_flag);
}

void dumpGem5Stats(const char* stats_desc) {
  ioctl(ALADDIN_FD, DUMP_STATS, stats_desc);
}

void resetTrace(unsigned req_code) {
  ioctl(ALADDIN_FD, RESET_TRACE, &req_code);
}

void resetGem5Stats() { ioctl(ALADDIN_FD, RESET_STATS, NULL); }

void mapArrayToAccelerator(unsigned req_code,
                           const char* array_name,
                           void* addr,
                           size_t size) {
  // Create the mapping struct to pass to the simulator.
  aladdin_map_t mapping;
  mapping.array_name = array_name;
  mapping.addr = addr;
  mapping.request_code = req_code;
  mapping.size = size;

  // We have to manually call the syscall instead of calling the wrapper
  // function fcntl() because it gets handled in libc in such a way that doesn't
  // actually invoke the fcntl syscall. fcntl doesn't seem to be in the vDSO, so
  // I'm not sure what's happening.
  syscall(SYS_fcntl, ALADDIN_FD, ALADDIN_MAP_ARRAY, &mapping);
}

void setArrayMemoryType(unsigned req_code,
                        const char* array_name,
                        MemoryType mem_type) {
  aladdin_mem_desc_t desc;
  desc.array_name = array_name;
  desc.request_code = req_code;
  desc.mem_type = mem_type;

  syscall(SYS_fcntl, ALADDIN_FD, ALADDIN_MEM_DESC, &desc);
}

const char* memoryTypeToString(MemoryType mem_type) {
  switch (mem_type) {
    case spad:
      return "spad";
    case reg:
      return "reg";
    case dma:
      return "dma";
    case acp:
      return "acp";
    case cache:
      return "cache";
    default:
      return "invalid";
  }
}

#ifdef __cplusplus
}  // extern "C"
#endif
