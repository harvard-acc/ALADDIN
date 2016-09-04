#include "stencil.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void stencil (TYPE orig[row_size * col_size], TYPE sol[row_size * col_size], TYPE filter[f_size]){
    int r, c, k1, k2;
    TYPE temp, mul;

#ifdef DMA_MODE
    dmaLoad(&orig[0], 0 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&orig[0], 1 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&orig[0], 2 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&orig[0], 3 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&orig[0], 4 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&orig[0], 5 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&orig[0], 6 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&orig[0], 7 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaLoad(&filter[0], 0, f_size * sizeof(TYPE));
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
    dmaStore(&sol[0], 0 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaStore(&sol[0], 1 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaStore(&sol[0], 2 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaStore(&sol[0], 3 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaStore(&sol[0], 4 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaStore(&sol[0], 5 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaStore(&sol[0], 6 * 1024 * sizeof(TYPE), PAGE_SIZE);
    dmaStore(&sol[0], 7 * 1024 * sizeof(TYPE), PAGE_SIZE);
#endif
}
