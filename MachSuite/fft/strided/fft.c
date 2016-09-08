#include "fft.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void fft(double real[FFT_SIZE], double img[FFT_SIZE],
         double real_twid[FFT_SIZE/2], double img_twid[FFT_SIZE/2]) {
    int even, odd, span, log, rootindex;
    double temp;

#ifdef DMA_MODE
    dmaLoad(&real[0], 0 * 512 * sizeof(double), 512 * sizeof(double));
    dmaLoad(&real[0], 1 * 512 * sizeof(double), 512 * sizeof(double));
    dmaLoad(&img[0], 0 * 512 * sizeof(double), 512 * sizeof(double));
    dmaLoad(&img[0], 1 * 512 * sizeof(double), 512 * sizeof(double));
    dmaLoad(&real_twid[0], 0, 512 * sizeof(double));
    dmaLoad(&img_twid[0], 0, 512 * sizeof(double));
#endif

    log = 0;

    outer:for(span=FFT_SIZE>>1; span; span>>=1, log++){
        inner:for(odd=span; odd<FFT_SIZE; odd++){
            odd |= span;
            even = odd ^ span;

            temp = real[even] + real[odd];
            real[odd] = real[even] - real[odd];
            real[even] = temp;

            temp = img[even] + img[odd];
            img[odd] = img[even] - img[odd];
            img[even] = temp;

            rootindex = (even<<log) & (FFT_SIZE - 1);
            if(rootindex){
                temp = real_twid[rootindex] * real[odd] -
                    img_twid[rootindex]  * img[odd];
                img[odd] = real_twid[rootindex]*img[odd] +
                    img_twid[rootindex]*real[odd];
                real[odd] = temp;
            }
        }
    }
#ifdef DMA_MODE
    dmaStore(&real[0], 0 * 512 * sizeof(double), 512 * sizeof(double));
    dmaStore(&real[0], 1 * 512 * sizeof(double), 512 * sizeof(double));
    dmaStore(&img[0], 0 * 512 * sizeof(double), 512 * sizeof(double));
    dmaStore(&img[0], 1 * 512 * sizeof(double), 512 * sizeof(double));
#endif
}
