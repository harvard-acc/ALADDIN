#include "sort.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void merge(TYPE a[SIZE], int start, int m, int stop){
    TYPE temp[SIZE];
    int i, j, k;

    merge_label1 : for(i=start; i<=m; i++){
        temp[i] = a[i];
    }

    merge_label2 : for(j=m+1; j<=stop; j++){
        temp[m+1+stop-j] = a[j];
    }

    i = start;
    j = stop;

    merge_label3 : for(k=start; k<=stop; k++){
        TYPE tmp_j = temp[j];
        TYPE tmp_i = temp[i];
        if(tmp_j < tmp_i) {
            a[k] = tmp_j;
            j--;
        } else {
            a[k] = tmp_i;
            i++;
        }
    }
}

void ms_mergesort(TYPE* host_a, TYPE* a) {
    int start, stop;
    int i, m, from, mid, to;

#ifdef DMA_MODE
    dmaLoad(a, host_a, SIZE * sizeof(TYPE));
#else
    a = host_a;
#endif

    start = 0;
    stop = SIZE;

    mergesort_label1 : for(m=1; m<stop-start; m+=m) {
        mergesort_label2 : for(i=start; i<stop; i+=m+m) {
            from = i;
            mid = i+m-1;
            to = i+m+m-1;
            if(to < stop){
                merge(a, from, mid, to);
            }
            else{
                merge(a, from, mid, stop);
            }
        }
    }
#ifdef DMA_MODE
    dmaStore(host_a, a, SIZE * sizeof(TYPE));
#endif
}
