/*
Based on algorithm described here:
http://www.cs.berkeley.edu/~mhoemmen/matrix-seminar/slides/UCB_sparse_tutorial_1.pdf
*/

#include "spmv.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void ellpack(TYPE nzval[N*L], int32_t cols[N*L], TYPE vec[N], TYPE out[N])
{
    int i, j;
    TYPE Si;

#ifdef DMA_MODE
    dmaLoad(&nzval[0], 0 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&nzval[0], 1 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&nzval[0], 2 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&nzval[0], 3 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&nzval[0], 4 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&nzval[0], 5 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&nzval[0], 6 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&nzval[0], 7 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&nzval[0], 8 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&nzval[0], 9 * 512 * sizeof(TYPE), 332 * sizeof(TYPE));
    dmaLoad(&cols[0], 0 * 1024 * sizeof(int32_t), PAGE_SIZE);
    dmaLoad(&cols[0], 1 * 1024 * sizeof(int32_t), PAGE_SIZE);
    dmaLoad(&cols[0], 2 * 1024 * sizeof(int32_t), PAGE_SIZE);
    dmaLoad(&cols[0], 3 * 1024 * sizeof(int32_t), PAGE_SIZE);
    dmaLoad(&cols[0], 4 * 1024 * sizeof(int32_t), 844 * sizeof(int32_t));
    dmaLoad(&vec[0], 0, N * sizeof(TYPE));
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
    dmaStore(&out[0], 0, N * sizeof(TYPE));
#endif
}
