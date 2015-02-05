#include <sys/ioctl.h>

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

}  // namespace ALADDIN
