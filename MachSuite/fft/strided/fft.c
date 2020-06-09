#include "fft.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void fft(double* host_real,
         double* host_img,
         double* host_real_twid,
         double* host_img_twid,
         double* real,
         double* img,
         double* real_twid,
         double* img_twid) {
    int even, odd, span, log, rootindex;
    double temp;

#ifdef DMA_MODE
    dmaLoad(real, host_real, FFT_SIZE * sizeof(double));
    dmaLoad(img, host_img, FFT_SIZE * sizeof(double));
    dmaLoad(real_twid, host_real_twid, (FFT_SIZE / 2) * sizeof(double));
    dmaLoad(img_twid, host_img_twid, (FFT_SIZE / 2) * sizeof(double));
#else
    real = host_real;
    img = host_img;
    real_twid = host_real_twid;
    img_twid = host_img_twid;
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
    dmaStore(host_real, real, FFT_SIZE * sizeof(double));
    dmaStore(host_img, img, FFT_SIZE * sizeof(double));
#endif
}
