#ifndef __ALADDIN_IOCTL_H__
#define __ALADDIN_IOCTL_H__

/* Support for invoking accelerators via ioctl() system call. A user level
 * program invokes an accelerator in the system like so:
 *
 *   ioctl(ALADDIN_FD, REQUEST_CODE);
 *
 * ALADDIN_FD is a special file descriptor reserved for Aladdin.
 * The request code specifies which accelerator to activate. This requires
 * agreement with the accelerator_id parameter used in the Python scripts to
 * bring up the system.
 * TODO: Register a callback function to be called when the accelerator
 * finishes, which we can then use to set up a pipeline of accelerators.
 */

namespace ALADDIN {

bool isValidRequestCode(unsigned req);

void invokeAccelerator(unsigned req_code, int* finish_flag);

};  // namespace ALADDIN

#endif
