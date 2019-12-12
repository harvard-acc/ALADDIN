#ifndef __ALADDIN_SYS_CONSTANTS_H__
#define __ALADDIN_SYS_CONSTANTS_H__

// ioctl() request codes for Aladdin.
extern unsigned MACHSUITE_AES_AES;
extern unsigned MACHSUITE_BACKPROP_BACKPROP;
extern unsigned MACHSUITE_BFS_BULK;
extern unsigned MACHSUITE_BFS_QUEUE;
extern unsigned MACHSUITE_FFT_STRIDED;
extern unsigned MACHSUITE_FFT_TRANSPOSE;
extern unsigned MACHSUITE_GEMM_BLOCKED;
extern unsigned MACHSUITE_GEMM_NCUBED;
extern unsigned MACHSUITE_KMP_KMP;
extern unsigned MACHSUITE_MD_GRID;
extern unsigned MACHSUITE_MD_KNN;
extern unsigned MACHSUITE_NW_NW;
extern unsigned MACHSUITE_SORT_MERGE;
extern unsigned MACHSUITE_SORT_RADIX;
extern unsigned MACHSUITE_SPMV_CRS;
extern unsigned MACHSUITE_SPMV_ELLPACK;
extern unsigned MACHSUITE_STENCIL_2D;
extern unsigned MACHSUITE_STENCIL_3D;
extern unsigned MACHSUITE_VITERBI_VITERBI;

extern unsigned SHOC_BBGEMM;
extern unsigned SHOC_FFT;
extern unsigned SHOC_MD;
extern unsigned SHOC_PPSCAN;
extern unsigned SHOC_REDUCTION;
extern unsigned SHOC_SSSORT;
extern unsigned SHOC_STENCIL;
extern unsigned SHOC_TRIAD;

extern unsigned CORTEXSUITE_STITCH;
extern unsigned CORTEXSUITE_LOCALIZATION;
extern unsigned CORTEXSUITE_SIFT;
extern unsigned CORTEXSUITE_DISPARITY;
extern unsigned CORTEXSUITE_DISPARITY_COMPUTESAD;
extern unsigned CORTEXSUITE_DISPARITY_INTEGRALIMAGE2D2D;
extern unsigned CORTEXSUITE_DISPARITY_FINALSAD;
extern unsigned CORTEXSUITE_DISPARITY_FINDDISPARITY;
extern unsigned CORTEXSUITE_MSER;
extern unsigned CORTEXSUITE_MULTI_NCUT;
extern unsigned CORTEXSUITE_SVM;
extern unsigned CORTEXSUITE_TEXTURE_SYNTHESIS;
extern unsigned CORTEXSUITE_TRACKING;
extern unsigned CORTEXSUITE_TRACKING_HARRIS;
extern unsigned CORTEXSUITE_TRACKING_AREASUM;
extern unsigned CORTEXSUITE_TRACKING_LAMBDA;

extern unsigned CORTEXSUITE_IMAGE_BLUR;
extern unsigned CORTEXSUITE_IMAGE_RESIZE;
extern unsigned CORTEXSUITE_SOBEL_DX;
extern unsigned CORTEXSUITE_SOBEL_DY;

extern unsigned PERFECT_WAMI_LK_STEEPEST;

// A special code used by integration tests.
extern unsigned INTEGRATION_TEST;

// A special file descriptor number in gem5 used for Aladdin.
extern int ALADDIN_FD;

// fcntl command that maps an array from the Aladdin trace to the simulator's
// address space, enabling memory writes from the accelerator to be visible to
// the simulated user program.
extern int ALADDIN_MAP_ARRAY;

// fcntl command that describes the memory access type for an array.
extern int ALADDIN_MEM_DESC;

// A preprovided sentinel value to detect changes in finish flags.
extern int NOT_COMPLETED;

// A special request code that triggers gem5 to dump all stats.
extern int DUMP_STATS;

// A special request code that triggers gem5 to reset all stats.
extern int RESET_STATS;

// A special request code to start reading a dynamic trace from the beginning
// again.
extern int RESET_TRACE;

// A special request code to suspend the CPU context if the value of the finish
// flag is equal to NOT_COMPLETED.
extern int WAIT_FINISH_SIGNAL;

// A string to pass to exitSimLoop to indicate that this is just a point to
// dump stats, not to end simulation.
extern const char* DUMP_STATS_EXIT_SIM_SIGNAL;

// A string to pass to exitSimLoop to indicate that this is just a point to
// reset gem5 stats (without dumping), not to end simulation.
extern const char* RESET_STATS_EXIT_SIM_SIGNAL;

#endif
