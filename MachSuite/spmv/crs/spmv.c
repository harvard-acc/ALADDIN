/*
Based on algorithm described here:
http://www.cs.berkeley.edu/~mhoemmen/matrix-seminar/slides/UCB_sparse_tutorial_1.pdf
*/

#include "spmv.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void spmv(TYPE* host_val,
          int32_t* host_cols,
          int32_t* host_rowDelimiters,
          TYPE* host_vec,
          TYPE* host_out,
          TYPE* val,
          int32_t* cols,
          int32_t* rowDelimiters,
          TYPE* vec,
          TYPE* out) {
    int i, j;
    TYPE sum, Si;

#ifdef DMA_MODE
    dmaLoad(rowDelimiters, host_rowDelimiters, (N + 1) * sizeof(int32_t));
    dmaLoad(vec, host_vec, N * sizeof(TYPE));
    dmaLoad(val, host_val, NNZ * sizeof(TYPE));
    dmaLoad(cols, host_cols, NNZ * sizeof(int32_t));
#else
    rowDelimiters = host_rowDelimiters;
    vec = host_vec;
    val = host_val;
    cols = host_cols;
    out = host_out;
#endif

    spmv_1 : for(i = 0; i < N; i++){
        sum = 0; Si = 0;
        int tmp_begin = rowDelimiters[i];
        int tmp_end = rowDelimiters[i+1];
        spmv_2 : for (j = tmp_begin; j < tmp_end; j++){
            Si = val[j] * vec[cols[j]];
            sum = sum + Si;
        }
        out[i] = sum;
    }
#ifdef DMA_MODE
    dmaStore(host_out, out, N * sizeof(TYPE));
#endif
}


