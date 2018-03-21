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

void invokeAcceleratorAndBlock(unsigned req_code) {
  int volatile finish_flag = NOT_COMPLETED;
  ioctl(ALADDIN_FD, req_code, &finish_flag);
  while (finish_flag == NOT_COMPLETED)
    ;
}

int* invokeAcceleratorAndReturn(unsigned req_code) {
  int* finish_flag = (int*)malloc(sizeof(int));
  *finish_flag = NOT_COMPLETED;
  ioctl(ALADDIN_FD, req_code, finish_flag);
  return finish_flag;
}

void invokeAcceleratorAndReturn2(unsigned req_code, volatile int* finish_flag) {
  *finish_flag = NOT_COMPLETED;
  ioctl(ALADDIN_FD, req_code, finish_flag);
}

void dumpGem5Stats(const char* stats_desc) {
  ioctl(ALADDIN_FD, DUMP_STATS, stats_desc);
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

#ifdef __cplusplus
}  // extern "C"
#endif
