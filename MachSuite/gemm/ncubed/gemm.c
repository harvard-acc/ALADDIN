#include "gemm.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void gemm(TYPE* host_m1, TYPE* host_m2, TYPE* host_prod,
            TYPE* m1, TYPE* m2, TYPE* prod) {
    int i, j, k;
    int k_col, i_col;
    TYPE mult;

#ifdef DMA_MODE
    dmaLoad(m1, host_m1, N * sizeof(TYPE));
    dmaLoad(m2, host_m2, N * sizeof(TYPE));
#else
    m1 = host_m1;
    m2 = host_m2;
    prod = host_prod;
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
    dmaStore(host_prod, prod, N * sizeof(TYPE));
#endif
}
