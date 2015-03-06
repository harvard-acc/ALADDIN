#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "aladdin_ioctl.h"
#include "aladdin_ioctl_req.h"

namespace ALADDIN {

bool isValidRequestCode(unsigned req) {
  // Checks if the last four bits are zero (to enforce the 16-value separation
  // rule) and if it is within the range of request codes specified.
  unsigned shifted = req >> 4;
  return ((req & 0xFFFFFFFF0) == req &&
          shifted >= 0x01 &&
          shifted <= 0x1B);
}

void invokeAccelerator(unsigned req_code, int* finish_flag) {
  ioctl(ALADDIN_FD, req_code, finish_flag);
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
  syscall(SYS_fcntl, ALADDIN_FD, MAP_ARRAY, &mapping);
}

}  // namespace ALADDIN
