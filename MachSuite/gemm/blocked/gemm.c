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

void bbgemm(TYPE m1[N], TYPE m2[N], TYPE prod[N]){
    int i, k, j, jj, kk;
    int i_row, k_row;
    TYPE temp_x, mul;

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
