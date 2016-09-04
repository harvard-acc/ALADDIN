#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include "viterbi.h"

// For testing, we can always choose the most likely probability instead of
// randomly sampling
//#define DETERMINISTIC_TRANSITION
//#define DETERMINISTIC_EMISSION
//#define DEBUG

void dump_transition(prob_t transition[N_STATES*N_STATES],int log) {
  state_t s0,s1;
  prob_t p;
  printf("transition\n");
  for(s0=0;s0<N_STATES;s0++) {
    printf("[ ");
    p = 0.0;
    for(s1=0;s1<N_STATES;s1++) {
      p += expf(-transition[s0*N_STATES+s1]);
      if(log)
        printf("%4.2f,", transition[s0*N_STATES+s1]);
      else
        printf("%01.2f,", expf(-transition[s0*N_STATES+s1]));
    }
    printf("] sum=%4.2f\n", p);
  }
}

void dump_emission(prob_t emission[N_STATES*N_TOKENS], int log) {
  state_t s;
  tok_t o;
  prob_t p;
  printf("emission\n");
  for(s=0;s<N_STATES;s++) {
    printf("[ ");
    p = 0.0;
    for(o=0;o<N_TOKENS;o++) {
      p += expf(-emission[s*N_TOKENS+o]);
      if(log)
        printf("%4.2f,", emission[s*N_TOKENS+o]);
      else
        printf("%01.2f,", expf(-emission[s*N_TOKENS+o]));
    }
    printf("] sum=%4.2f\n", p);
  }
}

void dump_init(prob_t init[N_STATES],int log) {
  state_t s;
  printf("init\n");
  printf("[ ");
  for(s=0;s<N_STATES;s++){
    if(log)
      printf("%4.2f,", init[s]);
    else
      printf("%01.2f,", expf(-init[s]));
  }
  printf("]\n");
}

void dump_path(state_t path[N_OBS]){
  step_t t;
  printf("path\n[");
  for(t=0; t<N_OBS; t++) {
    printf("%d,",path[t]);
  }
  printf("]\n");
}

void dump_obs(tok_t obs[N_OBS]){ 
  step_t t;
  printf("obs\n[");
  for(t=0; t<N_OBS; t++) {
    printf("%d,",obs[t]);
  }
  printf("]\n");
}

int main(int argc, char **argv) {
  struct bench_args_t data;
  int fd;
  state_t s0, s1, s[N_OBS];
  tok_t o;
  step_t t;
  double P[N_STATES];
  double Q[N_TOKENS];
  struct prng_rand_t state;
#if !defined(DETERMINISTIC_TRANSITION) || !defined(DETERMINISTIC_EMISSION)
  prob_t r;
#endif
#if defined(DETERMINISTIC_TRANSITION) || defined(DETERMINISTIC_TRANSITION)
  prob_t min;
  tok_t omin;
#endif

  // Fill data structure
  prng_srand(1,&state);

  // Generate a random transition matrix P(S1|S0)
  // Invariant: SUM_S1 P(S1|S0) = 1
  for(s0=0; s0<N_STATES; s0++) {
    // Generate random weights
    double sum = 0;
    for(s1=0; s1<N_STATES; s1++) {
      P[s1] = ((double)prng_rand(&state)/(double)PRNG_RAND_MAX);
#ifndef DETERMINISTIC_TRANSITION
      if(s1==s0) P[s1] = N_STATES; // self-transitions are much more likely
#else
      if(s1==s0) P[s1] = 0.5/N_STATES; // self-transitions are less likely (otherwise we'd never change states)
#endif
      sum += P[s1];
    }
    // Normalize and convert to -log domain
    for(s1=0; s1<N_STATES; s1++) {
      data.transition[s0*N_STATES+s1] = -1*logf(P[s1]/sum);
    }
  }

  // Generate a random emission matrix P(O|S)
  // Invariant: SUM_O P(O|S) = 1
  for(s0=0; s0<N_STATES; s0++) {
    // Generate random weights
    double sum = 0;
    for(o=0; o<N_TOKENS; o++) {
      Q[o] = ((double)prng_rand(&state)/(double)PRNG_RAND_MAX);
      if( o==s0 ) Q[o] = N_TOKENS; // one token is much more likely
      sum += Q[o];
    }
    // Normalize and convert to -log domain
    for(o=0; o<N_TOKENS; o++) {
      data.emission[s0*N_TOKENS+o] = -1*logf(Q[o]/sum);
    }
  }

  // Generate a random starting distribution P(S_0)
  // Invariant: SUM P(S_0) = 1
  {
    // Generate random weights
    double sum = 0;
    for(s0=0; s0<N_STATES; s0++) {
      P[s0] = ((double)prng_rand(&state)/(double)PRNG_RAND_MAX);
      sum += P[s0];
    }
    // Normalize and convert to -log domain
    for(s0=0; s0<N_STATES; s0++) {
      data.init[s0] = -1*logf(P[s0]/sum);
    }
  }

  #ifdef DEBUG
  dump_init(data.init,0);
  dump_transition(data.transition,0);
  dump_emission(data.emission,0);
  #endif

  // To get observations, just run the HMM forwards N_OBS steps;
  // Nondeterministic sampling uses the inverse transform method

  // Sample s_0 from init
#ifndef DETERMINISTIC_TRANSITION
  r = ((double)prng_rand(&state)/(double)PRNG_RAND_MAX);
  s[0]=0; do{r-=expf(-data.init[s[0]]);} while(r>0&&(++s[0]));
#else
  s[0]=0;
  min=data.init[0];
  for(s0=1;s0<N_STATES;s0++) {
    if(data.init[s0]<min) {
      min=data.init[s0];
      s[0]=s0;
    }
  }
#endif
  // Sample o_0 from emission
#ifndef DETERMINISTIC_EMISSION
  r = ((double)prng_rand(&state)/(double)PRNG_RAND_MAX);
  for( o=0; r>0; ++o ) {r-=expf(-data.emission[s[0]*N_TOKENS+o]);}
  o=0; do{ r-=expf(-data.emission[s[0]*N_TOKENS+o]); }while(r>0 && ++o);
  data.obs[0] = o;
#else
  omin=0;
  min=data.emission[s[0]*N_TOKENS+0];
  for(o=1;o<N_TOKENS;o++) {
    if(data.emission[s[0]*N_TOKENS+o]<min) {
      min=data.emission[s[0]*N_TOKENS+o];
      omin=o;
    }
  }
  data.obs[0] = omin;
#endif

  for(t=1; t<N_OBS; t++) {
    // Sample s_t from transition
#ifndef DETERMINISTIC_TRANSITION
    r = ((double)prng_rand(&state)/(double)PRNG_RAND_MAX);
    s[t]=0; do{r-=expf(-data.transition[s[t-1]*N_STATES+s[t]]);} while(r>0&&++s[t]);
#else
    s[t]=0;
    min=data.transition[s[t-1]*N_STATES];
    for(s0=1;s0<N_STATES;s0++) {
      if(data.transition[s[t-1]*N_STATES+s0]<min) {
        min=data.transition[s[t-1]*N_STATES+s0];
        s[t]=s0;
      }
    }
#endif
    // Sample o_t from emission
#ifndef DETERMINISTIC_EMISSION
    r = ((double)prng_rand(&state)/(double)PRNG_RAND_MAX);
    o=0; do{ r-=expf(-data.emission[s[t]*N_TOKENS+o]); }while(r>0 && ++o);
    data.obs[t] = o;
#else
    omin=0;
    min=data.emission[s[t]*N_TOKENS+0];
    for(o=1;o<N_TOKENS;o++) {
      if(data.emission[s[t]*N_TOKENS+o]<min) {
        min=data.emission[s[t]*N_TOKENS+o];
        omin=o;
      }
    }
    data.obs[t] = omin;
#endif
  }

  #ifdef DEBUG
  dump_path(s);
  dump_obs(data.obs);
  #endif

  // Open and write
  fd = open("input.data", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  assert( fd>0 && "Couldn't open input data file" );
  data_to_input(fd, (void*)(&data));

  return 0;
}
