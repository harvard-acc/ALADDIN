/*
Implementation based on http://www-igm.univ-mlv.fr/~lecroq/string/node8.html
*/

#include "kmp.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void CPF(char pattern[PATTERN_SIZE], int32_t kmpNext[PATTERN_SIZE]) {
    int32_t k, q;
    k = 0;
    kmpNext[0] = 0;

    c1 : for(q = 1; q < PATTERN_SIZE; q++){
        c2 : while(k > 0 && pattern[k] != pattern[q]){
            k = kmpNext[q];
        }
        if(pattern[k] == pattern[q]){
            k++;
        }
        kmpNext[q] = k;
    }
}


int kmp(char pattern[PATTERN_SIZE], char input[STRING_SIZE], int32_t kmpNext[PATTERN_SIZE], int32_t n_matches[1]) {
    int32_t i, q;

#ifdef DMA_MODE
    dmaLoad(&pattern[0], 0, PATTERN_SIZE*sizeof(char));
    dmaLoad(&kmpNext[0], 0, PATTERN_SIZE*sizeof(int32_t));
    dmaLoad(&input[0], 0*4096, PAGE_SIZE);
    dmaLoad(&input[0], 1*4096, PAGE_SIZE);
    dmaLoad(&input[0], 2*4096, PAGE_SIZE);
    dmaLoad(&input[0], 3*4096, PAGE_SIZE);
    dmaLoad(&input[0], 4*4096, PAGE_SIZE);
    dmaLoad(&input[0], 5*4096, PAGE_SIZE);
    dmaLoad(&input[0], 6*4096, PAGE_SIZE);
    dmaLoad(&input[0], 7*4096, 3739);
#endif
    n_matches[0] = 0;

    CPF(pattern, kmpNext);

    q = 0;
    k1 : for(i = 0; i < STRING_SIZE; i++){
        k2 : while (q > 0 && pattern[q] != input[i]){
            q = kmpNext[q];
        }
        if (pattern[q] == input[i]){
            q++;
        }
        if (q >= PATTERN_SIZE){
            n_matches[0]++;
            q = kmpNext[q - 1];
        }
    }
    return 0;
}
