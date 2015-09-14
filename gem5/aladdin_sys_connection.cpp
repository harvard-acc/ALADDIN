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

bool isValidRequestCode(unsigned req) {
  // Checks if the last four bits are zero (to enforce the 16-value separation
  // rule) and if it is within the range of request codes specified.

  unsigned shifted = req >> 4;
  return ((req & 0xFFFFFFFF0) == req &&
          shifted >= 0x01 &&
          shifted <= 0x28) ||
          (req == INTEGRATION_TEST);
}

void invokeAcceleratorAndBlock(unsigned req_code) {
  int volatile finish_flag = NOT_COMPLETED;
  ioctl(ALADDIN_FD, req_code, &finish_flag);
  while (finish_flag == NOT_COMPLETED)
    ;
}

int* invokeAcceleratorAndReturn(unsigned req_code) {
  int* finish_flag = (int*) malloc(sizeof(int));
  *finish_flag = NOT_COMPLETED;
  ioctl(ALADDIN_FD, req_code, finish_flag);
  return finish_flag;
}

void mapArrayToAccelerator(
    unsigned req_code, const char* array_name, void* addr, size_t size) {
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

#ifdef __cplusplus
}  // extern "C"
#endif
