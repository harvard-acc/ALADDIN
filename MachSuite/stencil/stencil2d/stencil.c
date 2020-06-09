#include "stencil.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void stencil(TYPE* host_orig,
             TYPE* host_sol,
             TYPE* orig,
             TYPE* sol,
             TYPE* filter) {
    int r, c, k1, k2;
    TYPE temp, mul;

#ifdef DMA_MODE
    dmaLoad(orig, host_orig, row_size * col_size * sizeof(TYPE));
    // This is used to zero-initialize the array.
    dmaLoad(sol, host_sol, row_size * col_size * sizeof(TYPE));
#else
    orig = host_orig;
    sol = host_sol;
#endif

    stencil_label1:for (r=0; r<row_size-2; r++) {
        stencil_label2:for (c=0; c<col_size-2; c++) {
            temp = (TYPE)0;
            stencil_label3:for (k1=0;k1<3;k1++){
                stencil_label4:for (k2=0;k2<3;k2++){
                    mul = filter[k1*3 + k2] * orig[(r+k1)*col_size + c+k2];
                    temp += mul;
                }
            }
            sol[(r*col_size) + c] = temp;
        }
    }

#ifdef DMA_MODE
    dmaStore(host_sol, sol, row_size * col_size * sizeof(TYPE));
#endif
}
