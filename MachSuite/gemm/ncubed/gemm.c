#include "gemm.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void gemm( TYPE m1[N], TYPE m2[N], TYPE prod[N] ){
    int i, j, k;
    int k_col, i_col;
    TYPE mult;

#ifdef DMA_MODE
    dmaLoad(&m1[0], 0 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m1[0], 1 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m1[0], 2 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m1[0], 3 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m1[0], 4 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m1[0], 5 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m1[0], 6 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m1[0], 7 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m2[0], 0 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m2[0], 1 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m2[0], 2 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m2[0], 3 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m2[0], 4 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m2[0], 5 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m2[0], 6 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&m2[0], 7 * 512 * sizeof(TYPE), PAGE_SIZE);
#endif

    outer:for(i=0;i<row_size;i++) {
        middle:for(j=0;j<col_size;j++) {
            i_col = i * col_size;
            TYPE sum = 0;
            inner:for(k=0;k<row_size;k++) {
                k_col = k * col_size;
                mult = m1[i_col + k] * m2[k_col + j];
                sum += mult;
            }
            prod[i_col + j]  = sum;
        }
    }

#ifdef DMA_MODE
    dmaStore(&prod[0], 0 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaStore(&prod[0], 1 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaStore(&prod[0], 2 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaStore(&prod[0], 3 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaStore(&prod[0], 4 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaStore(&prod[0], 5 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaStore(&prod[0], 6 * 512 * sizeof(TYPE), PAGE_SIZE);
    dmaStore(&prod[0], 7 * 512 * sizeof(TYPE), PAGE_SIZE);
#endif
}
