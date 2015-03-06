#ifndef __ALADDIN_IOCTL_REQ_H__
#define __ALADDIN_IOCTL_REQ_H__

// ioctl() request codes for Aladdin.

namespace ALADDIN {

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

extern int ALADDIN_FD;

extern int MAP_ARRAY;

}  // namespace ALADDIN

#endif
