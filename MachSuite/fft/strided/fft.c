#include "fft.h"
#include "gem5/dma_interface.h"
void fft(double real[NUM], double img[NUM], double real_twid[NUM], double img_twid[NUM]){
#ifdef DMA_MODE
  dmaLoad(&real[0],1024*8*8);
  dmaLoad(&img[0],1024*8*8);
  dmaLoad(&real_twid[0],1024*8*8);
  dmaLoad(&img_twid[0],1024*8*8);
#endif
    int even, odd, span, log, rootindex;
    double temp;
    log = 0;

    for(span=NUM>>1; span; span>>=1, log++){
        for(odd=span; odd<NUM; odd++){
            odd |= span;
            even = odd ^ span;

            temp = real[even] + real[odd];
            real[odd] = real[even] - real[odd];
            real[even] = temp;

            temp = img[even] + img[odd];
            img[odd] = img[even] - img[odd];
            img[even] = temp;

            rootindex = (even<<log) & (NUM - 1);
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
  dmaStore(&real[0],1024*8*8);
  dmaStore(&img[0],1024*8*8);
#endif
}
