#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define THREADS 64

//#define TYPE double
//#define TYPE float
#define TYPE int

/*typedef struct complex_t {*/
/*TYPE x;*/
/*TYPE y;*/
/*} complex;*/

//#define M_PI 3.14159265358979323846f
#define MM_SQRT1_2      0.70710678118654752440f

#define THREADS 64

#define cmplx_M_x(a_x, a_y, b_x, b_y) (a_x*b_x - a_y *b_y)
#define cmplx_M_y(a_x, a_y, b_x, b_y) (a_x*b_y + a_y *b_x)
#define cmplx_MUL_x(a_x, a_y, b_x, b_y ) (a_x*b_x - a_y*b_y)
#define cmplx_MUL_y(a_x, a_y, b_x, b_y ) (a_x*b_y + a_y*b_x)
#define cmplx_mul_x(a_x, a_y, b_x, b_y) (a_x*b_x - a_y*b_y)
#define cmplx_mul_y(a_x, a_y, b_x, b_y) (a_x*b_y + a_y*b_x)
#define cmplx_add_x(a_x, b_x) (a_x + b_x)
#define cmplx_add_y(a_y, b_y) (a_y + b_y)
#define cmplx_sub_x(a_x, b_x) (a_x - b_x)
#define cmplx_sub_y(a_y, b_y) (a_y - b_y)
#define cm_fl_mul_x(a_x, b) (b*a_x)
#define cm_fl_mul_y(a_y, b) (b*a_y)

#define store8(a_x, x, offset, sx,reversed){ \
    x[0*sx+offset] = a_x[reversed[0]]; \
    x[1*sx+offset] = a_x[reversed[1]]; \
    x[2*sx+offset] = a_x[reversed[2]]; \
    x[3*sx+offset] = a_x[reversed[3]]; \
    x[4*sx+offset] = a_x[reversed[4]]; \
    x[5*sx+offset] = a_x[reversed[5]]; \
    x[6*sx+offset] = a_x[reversed[6]]; \
    x[7*sx+offset] = a_x[reversed[7]]; \
}

#define load8(a_x, x, offset, sx){ \
    a_x[0] = x[0*sx+offset]; \
    a_x[1] = x[1*sx+offset]; \
    a_x[2] = x[2*sx+offset]; \
    a_x[3] = x[3*sx+offset]; \
    a_x[4] = x[4*sx+offset]; \
    a_x[5] = x[5*sx+offset]; \
    a_x[6] = x[6*sx+offset]; \
    a_x[7] = x[7*sx+offset]; \
}

#define FF2(a0_x, a0_y, a1_x, a1_y){			\
	TYPE c0_x = *a0_x;		\
	TYPE c0_y = *a0_y;		\
	*a0_x = cmplx_add_x(c0_x, *a1_x);	\
	*a0_y = cmplx_add_y(c0_y, *a1_y);	\
	*a1_x = cmplx_sub_x(c0_x, *a1_x);	\
	*a1_y = cmplx_sub_y(c0_y, *a1_y);	\
}

#define FFT4(a0_x, a0_y, a1_x, a1_y, a2_x, a2_y, a3_x, a3_y){           \
	TYPE exp_1_44_x;		\
	TYPE exp_1_44_y;		\
	TYPE tmp;			\
	exp_1_44_x =  0.0;		\
	exp_1_44_y =  -1.0;		\
	FF2( a0_x, a0_y, a2_x, a2_y);   \
	FF2( a1_x, a1_y, a3_x, a3_y);   \
	tmp = *a3_x;			\
	*a3_x = *a3_x*exp_1_44_x-*a3_y*exp_1_44_y;     	\
	*a3_y = tmp*exp_1_44_y - *a3_y*exp_1_44_x;    	\
	FF2( a0_x, a0_y, a1_x, a1_y );                  \
	FF2( a2_x, a2_y, a3_x, a3_y );                  \
}

#define FFT8(a_x, a_y)			\
{                                               \
	TYPE exp_1_8_x, exp_1_4_x, exp_3_8_x;	\
	TYPE exp_1_8_y, exp_1_4_y, exp_3_8_y;	\
	TYPE tmp_1, tmp_2;			\
	exp_1_8_x =  1;				\
	exp_1_8_y = -1;				\
	exp_1_4_x =  0;				\
	exp_1_4_y = -1;				\
	exp_3_8_x = -1;				\
	exp_3_8_y = -1;				\
	FF2( &a_x[0], &a_y[0], &a_x[4], &a_y[4]);			\
	FF2( &a_x[1], &a_y[1], &a_x[5], &a_y[5]);			\
	FF2( &a_x[2], &a_y[2], &a_x[6], &a_y[6]);			\
	FF2( &a_x[3], &a_y[3], &a_x[7], &a_y[7]);			\
	tmp_1 = a_x[5];							\
	a_x[5] = cm_fl_mul_x( cmplx_mul_x(a_x[5], a_y[5], exp_1_8_x, exp_1_8_y),  MM_SQRT1_2 );	\
	a_y[5] = cm_fl_mul_y( cmplx_mul_y(tmp_1, a_y[5], exp_1_8_x, exp_1_8_y) , MM_SQRT1_2 );	\
	tmp_1 = a_x[6];							\
	a_x[6] = cmplx_mul_x( a_x[6], a_y[6], exp_1_4_x , exp_1_4_y);	\
	a_y[6] = cmplx_mul_y( tmp_1, a_y[6], exp_1_4_x , exp_1_4_y);	\
	tmp_1 = a_x[7];							\
	a_x[7] = cm_fl_mul_x( cmplx_mul_x(a_x[7], a_y[7], exp_3_8_x, exp_3_8_y), MM_SQRT1_2 );	\
	a_y[7] = cm_fl_mul_y( cmplx_mul_y(tmp_1, a_y[7], exp_3_8_x, exp_3_8_y) , MM_SQRT1_2 );	\
	FFT4( &a_x[0], &a_y[0], &a_x[1], &a_y[1], &a_x[2], &a_y[2], &a_x[3], &a_y[3] );	\
	FFT4( &a_x[4], &a_y[4], &a_x[5], &a_y[5], &a_x[6], &a_y[6], &a_x[7], &a_y[7] );	\
}

void fft1D_512(TYPE work_x[512], TYPE work_y[512],
	TYPE DATA_x[THREADS*8],
	TYPE DATA_y[THREADS*8],
	TYPE data_x[ 8 ],
	TYPE data_y[ 8 ],
	TYPE smem[8*8*9],
  int reversed[8],
  float sin_64[448],
  float cos_64[448],
  float sin_512[448],
  float cos_512[448]
);

