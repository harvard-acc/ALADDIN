#include "viterbi.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

int viterbi( tok_t obs[N_OBS], prob_t init[N_STATES], prob_t transition[N_STATES*N_STATES], prob_t emission[N_STATES*N_TOKENS], state_t path[N_OBS] )
{
  prob_t llike[N_OBS][N_STATES];
  step_t t;
  state_t prev, curr;
  prob_t min_p, p;
  state_t min_s, s;
  // All probabilities are in -log space. (i.e.: P(x) => -log(P(x)) )

#ifdef DMA_MODE
  dmaLoad(&obs[0], 0, N_OBS * sizeof(tok_t));
  dmaLoad(&init[0], 0, N_STATES * sizeof(prob_t));
  dmaLoad(&transition[0], 0 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&transition[0], 1 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&transition[0], 2 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&transition[0], 3 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&transition[0], 4 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&transition[0], 5 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&transition[0], 6 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&transition[0], 7 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&emission[0], 0 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&emission[0], 1 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&emission[0], 2 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&emission[0], 3 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&emission[0], 4 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&emission[0], 5 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&emission[0], 6 * 512 * sizeof(prob_t), PAGE_SIZE);
  dmaLoad(&emission[0], 7 * 512 * sizeof(prob_t), PAGE_SIZE);
#endif

  // Initialize with first observation and initial probabilities
  L_init: for( s=0; s<N_STATES; s++ ) {
    llike[0][s] = init[s] + emission[s*N_TOKENS+obs[0]];
  }

  // Iteratively compute the probabilities over time
  L_timestep: for( t=1; t<N_OBS; t++ ) {
    L_curr_state: for( curr=0; curr<N_STATES; curr++ ) {
      // Compute likelihood HMM is in current state and where it came from.
      prev = 0;
      min_p = llike[t-1][prev] +
              transition[prev*N_STATES+curr] +
              emission[curr*N_TOKENS+obs[t]];
      L_prev_state: for( prev=1; prev<N_STATES; prev++ ) {
        p = llike[t-1][prev] +
            transition[prev*N_STATES+curr] +
            emission[curr*N_TOKENS+obs[t]];
        if( p<min_p ) {
          min_p = p;
        }
      }
      llike[t][curr] = min_p;
    }
  }

  // Identify end state
  min_s = 0;
  min_p = llike[N_OBS-1][min_s];
  L_end: for( s=1; s<N_STATES; s++ ) {
    p = llike[N_OBS-1][s];
    if( p<min_p ) {
      min_p = p;
      min_s = s;
    }
  }
  path[N_OBS-1] = min_s;

  // Backtrack to recover full path
  L_backtrack: for( t=N_OBS-2; t>=0; t-- ) {
    min_s = 0;
    min_p = llike[t][min_s] + transition[min_s*N_STATES+path[t+1]];
    L_state: for( s=1; s<N_STATES; s++ ) {
      p = llike[t][s] + transition[s*N_STATES+path[t+1]];
      if( p<min_p ) {
        min_p = p;
        min_s = s;
      }
    }
    path[t] = min_s;
  }

#ifdef DMA_MODE
  dmaStore(&path[0], 0, N_OBS * sizeof(state_t));
#endif

  return 0;
}

