/*
Based on algorithm described here:
http://www.cs.berkeley.edu/~mhoemmen/matrix-seminar/slides/UCB_sparse_tutorial_1.pdf
*/

#include "spmv.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void spmv(TYPE val[NNZ], int32_t cols[NNZ], int32_t rowDelimiters[N+1], TYPE vec[N], TYPE out[N]) {
    int i, j;
    TYPE sum, Si;

#ifdef DMA_MODE
    dmaLoad(&rowDelimiters[0], 0, (N + 1) * sizeof(int32_t));
    dmaLoad(&vec[0], 0, N * sizeof(TYPE));
    dmaLoad(&val[0], 0 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&val[0], 1 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&val[0], 2 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&val[0], 3 * 512 * sizeof(TYPE), 130 * sizeof(TYPE));
    dmaLoad(&cols[0], 0 * 1024 * sizeof(int32_t), PAGE_SIZE);
    dmaLoad(&cols[0], 1 * 1024 * sizeof(int32_t), 642 * sizeof(int32_t));
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
    dmaStore(&out[0], 0, N * sizeof(TYPE));
#endif
}


