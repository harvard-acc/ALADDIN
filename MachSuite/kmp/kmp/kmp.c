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


int kmp(char* host_input,
        int32_t* host_n_matches,
        char* pattern,
        char* input,
        int32_t* kmpNext,
        int32_t* n_matches) {
    int32_t i, q;

#ifdef DMA_MODE
    // The pattern and kmpNext arrays are so small that they should always be
    // mapped to registers, not SRAM, in which case they don't need to be
    // loaded.
    dmaLoad(input, host_input, STRING_SIZE * sizeof(char));
#else
    input = host_input;
    n_matches = host_n_matches;
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
#ifdef DMA_MODE
    // The n_matches array is also too small for SRAM, but there's no other way
    // to get the data out of the accelerator right now besides a DMA store
    // from a scratchpad.
    dmaStore(host_n_matches, n_matches, sizeof(int32_t));
#endif
    return 0;
}
