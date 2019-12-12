/*
 * Aladdin to system connection constants.
 *
 * These include ioctl request codes and other command numbers.
 *
 * Each code represents a benchmark. When assigning request codes, separate
 * adjacent codes by at least 16 (0x10). This is to enable a benchmark to
 * assign distinct request codes for individual kernels if desired without
 * colliding into another benchmark.  See the CORTEXSUITE definitions below for
 * an example of this.
 */

#include "aladdin_sys_constants.h"

unsigned MACHSUITE_AES_AES           = 0x00000010;
unsigned MACHSUITE_BACKPROP_BACKPROP = 0x00000020;
unsigned MACHSUITE_BFS_BULK          = 0x00000030;
unsigned MACHSUITE_BFS_QUEUE         = 0x00000040;
unsigned MACHSUITE_FFT_STRIDED       = 0x00000050;
unsigned MACHSUITE_FFT_TRANSPOSE     = 0x00000060;
unsigned MACHSUITE_GEMM_BLOCKED      = 0x00000070;
unsigned MACHSUITE_GEMM_NCUBED       = 0x00000080;
unsigned MACHSUITE_KMP_KMP           = 0x00000090;
unsigned MACHSUITE_MD_GRID           = 0x000000A0;
unsigned MACHSUITE_MD_KNN            = 0x000000B0;
unsigned MACHSUITE_NW_NW             = 0x000000C0;
unsigned MACHSUITE_SORT_MERGE        = 0x000000D0;
unsigned MACHSUITE_SORT_RADIX        = 0x000000E0;
unsigned MACHSUITE_SPMV_CRS          = 0x000000F0;
unsigned MACHSUITE_SPMV_ELLPACK      = 0x00000100;
unsigned MACHSUITE_STENCIL_2D        = 0x00000110;
unsigned MACHSUITE_STENCIL_3D        = 0x00000120;
unsigned MACHSUITE_VITERBI_VITERBI   = 0x00000130;

unsigned SHOC_BBGEMM    = 0x00000140;
unsigned SHOC_FFT       = 0x00000150;
unsigned SHOC_MD        = 0x00000160;
unsigned SHOC_PPSCAN    = 0x00000170;
unsigned SHOC_REDUCTION = 0x00000180;
unsigned SHOC_SSSORT    = 0x00000190;
unsigned SHOC_STENCIL   = 0x000001A0;
unsigned SHOC_TRIAD     = 0x000001B0;

unsigned CORTEXSUITE_STITCH       = 0x000001C0;
unsigned CORTEXSUITE_LOCALIZATION = 0x000001D0;
unsigned CORTEXSUITE_SIFT         = 0x000001E0;
unsigned CORTEXSUITE_DISPARITY                   = 0x000001F0;
unsigned CORTEXSUITE_DISPARITY_COMPUTESAD        = 0x000001F1;
unsigned CORTEXSUITE_DISPARITY_INTEGRALIMAGE2D2D = 0x000001F2;
unsigned CORTEXSUITE_DISPARITY_FINALSAD          = 0x000001F3;
unsigned CORTEXSUITE_DISPARITY_FINDDISPARITY     = 0x000001F4;
unsigned CORTEXSUITE_MSER              = 0x00000200;
unsigned CORTEXSUITE_MULTI_NCUT        = 0x00000210;
unsigned CORTEXSUITE_SVM               = 0x00000220;
unsigned CORTEXSUITE_TEXTURE_SYNTHESIS = 0x00000230;
unsigned CORTEXSUITE_TRACKING          = 0x00000240;
unsigned CORTEXSUITE_TRACKING_HARRIS   = 0x00000241;
unsigned CORTEXSUITE_TRACKING_AREASUM  = 0x00000242;
unsigned CORTEXSUITE_TRACKING_LAMBDA   = 0x00000243;

unsigned CORTEXSUITE_IMAGE_BLUR        = 0x00000250;
unsigned CORTEXSUITE_IMAGE_RESIZE      = 0x00000260;
unsigned CORTEXSUITE_SOBEL_DX          = 0x00000270;
unsigned CORTEXSUITE_SOBEL_DY          = 0x00000280;

unsigned PERFECT_WAMI_LK_STEEPEST = 0x00000290;

unsigned INTEGRATION_TEST = 0x01000000;

int ALADDIN_FD         = 0x000DECAF;
int ALADDIN_MAP_ARRAY  = 0x001DECAF;
int NOT_COMPLETED      = 0x002DECAF;
int DUMP_STATS         = 0x003DECAF;
int RESET_STATS        = 0x004DECAF;
int RESET_TRACE        = 0x005DECAF;
int ALADDIN_MEM_DESC   = 0x006DECAF;
int WAIT_FINISH_SIGNAL = 0x007DECAF;

const char* DUMP_STATS_EXIT_SIM_SIGNAL = "statistics_dump:";
const char* RESET_STATS_EXIT_SIM_SIGNAL = "statistics_reset:";
