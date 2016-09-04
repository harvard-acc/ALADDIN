#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "md.h"

#define domainEdge 20.0
// LJ1=4e(s^12), LJ2=4e(s^6) -> s=1.049
#define van_der_Waals_thresh (1.049*1.049)

static inline TYPE dist_sq(TYPE x1, TYPE y1, TYPE z1, TYPE x2, TYPE y2, TYPE z2) {
  TYPE dx, dy, dz;
  dx=x2-x1;
  dy=y2-y1;
  dz=z2-z1;
  return dx*dx + dy*dy + dz*dz;
}

typedef struct {
  int index;
  TYPE dist_sq;
} neighbor_t;

int neighbor_compar(const void *v_lhs, const void *v_rhs) {
  neighbor_t lhs = *((neighbor_t *)v_lhs);
  neighbor_t rhs = *((neighbor_t *)v_rhs);
  return lhs.dist_sq==rhs.dist_sq ? 0 : ( lhs.dist_sq<rhs.dist_sq ? -1 : 1 );
}

int main(int argc, char **argv)
{
  struct bench_args_t data;
  int i, j, reject, fd;
  neighbor_t neighbor_list[nAtoms];
  TYPE x, y, z;
  const TYPE infinity = (domainEdge*domainEdge*3.)*1000;//(max length)^2 * 1000
  struct prng_rand_t state;

  // Create random positions in the box [0,domainEdge]^3
  prng_srand(1,&state);
  i=0;
  while( i<nAtoms ) {
    // Generate a new point
    x = domainEdge*(((TYPE)prng_rand(&state))/((TYPE)PRNG_RAND_MAX));
    y = domainEdge*(((TYPE)prng_rand(&state))/((TYPE)PRNG_RAND_MAX));
    z = domainEdge*(((TYPE)prng_rand(&state))/((TYPE)PRNG_RAND_MAX));
    // Assure that it's not directly on top of another atom
    reject = 0;
    for( j=0; j<i; j++ ) {
      if( dist_sq(x,y,z, data.position_x[j], data.position_y[j], data.position_z[j])<van_der_Waals_thresh ) {
        reject=1;
        break;
      }
    }
    if(!reject) {
      data.position_x[i] = x;
      data.position_y[i] = y;
      data.position_z[i] = z;
      //printf("%lf %lf %lf\n", data.position_x[i], data.position_y[i], data.position_z[i] );
      ++i;
    }
  }

  // Compute k-nearest neighbors
  memset(data.NL, 0, nAtoms*maxNeighbors*sizeof(int32_t));
  for( i=0; i<nAtoms; i++ ) {
    for( j=0; j<nAtoms; j++ ) {
      neighbor_list[j].index = j;
      if( i==j )
        neighbor_list[j].dist_sq = infinity;
      else
        neighbor_list[j].dist_sq = dist_sq(data.position_x[i], data.position_y[i], data.position_z[i], data.position_x[j], data.position_y[j], data.position_z[j]);
    }
    qsort(neighbor_list, nAtoms, sizeof(neighbor_t), neighbor_compar);
    //printf("%d:", i);
    for( j=0; j<maxNeighbors; j++ ) {
      data.NL[i*maxNeighbors +j] = neighbor_list[j].index;
      //printf(" {%d,%lf}", neighbor_list[j].index, neighbor_list[j].dist_sq);
    }
    //printf("\n\n");
  }

  // Open and write
  fd = open("input.data", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  assert( fd>0 && "Couldn't open input data file" );
  data_to_input(fd, (void *)(&data));

  return 0;
}
