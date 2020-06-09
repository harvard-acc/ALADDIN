/*
Implementation based on algorithm described in:
The cache performance and optimizations of blocked algorithms
M. D. Lam, E. E. Rothberg, and M. E. Wolf
ASPLOS 1991
*/

#include "gemm.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void bbgemm(TYPE* host_m1, TYPE* host_m2, TYPE* host_prod,
            TYPE* m1, TYPE* m2, TYPE* prod) {
    int i, k, j, jj, kk;
    int i_row, k_row;
    TYPE temp_x, mul;

#ifdef DMA_MODE
    dmaLoad(m1, host_m1, N * sizeof(TYPE));
    dmaLoad(m2, host_m2, N * sizeof(TYPE));
#else
    m1 = host_m1;
    m2 = host_m2;
    prod = host_prod;
#endif

    loopjj:for (jj = 0; jj < row_size; jj += block_size){
        loopkk:for (kk = 0; kk < row_size; kk += block_size){
            loopi:for ( i = 0; i < row_size; ++i){
                loopk:for (k = 0; k < block_size; ++k){
                    i_row = i * row_size;
                    k_row = (k  + kk) * row_size;
                    temp_x = m1[i_row + k + kk];
                    loopj:for (j = 0; j < block_size; ++j){
                        mul = temp_x * m2[k_row + j + jj];
                        prod[i_row + j + jj] += mul;
                    }
                }
            }
        }
    }
#ifdef DMA_MODE
    dmaStore(host_prod, prod, N * sizeof(TYPE));
#endif
}
