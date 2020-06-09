/*
Based on algorithm described here:
http://www.cs.berkeley.edu/~mhoemmen/matrix-seminar/slides/UCB_sparse_tutorial_1.pdf
*/

#include "spmv.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void ellpack(TYPE* host_nzval,
             int32_t* host_cols,
             TYPE* host_vec,
             TYPE* host_out,
             TYPE* nzval,
             int32_t* cols,
             TYPE* vec,
             TYPE* out)
{
    int i, j;
    TYPE Si;

#ifdef DMA_MODE
    dmaLoad(nzval, host_nzval, (N*L) * sizeof(TYPE));
    dmaLoad(cols, host_cols, (N*L) * sizeof(int32_t));
    dmaLoad(vec, host_vec, (N) * sizeof(TYPE));
#else
    nzval = host_nzval;
    cols = host_cols;
    vec = host_vec;
    out = host_out;
#endif

    ellpack_1 : for (i=0; i<N; i++) {
        TYPE sum = out[i];
        ellpack_2 : for (j=0; j<L; j++) {
                Si = nzval[j + i*L] * vec[cols[j + i*L]];
                sum += Si;
        }
        out[i] = sum;
    }

#ifdef DMA_MODE
    dmaStore(host_out, out, N * sizeof(TYPE));
#endif
}
