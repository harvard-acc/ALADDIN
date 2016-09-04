#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "md.h"

// LJ1=4e(s^12), LJ2=4e(s^6) -> s=1.049
#define van_der_Waals_thresh (1.049*1.049)

static inline TYPE dist_sq( dvector_t p, dvector_t q ) {
  TYPE dx, dy, dz;
  dx = p.x - q.x;
  dy = p.y - q.y;
  dz = p.z - q.z;
  return dx*dx + dy*dy + dz*dz;
}

int main(int argc, char **argv)
{
  struct bench_args_t data;
  int fd, reject;
  int32_t i;
  dvector_t p, q;
  ivector_t b;
  dvector_t points[nAtoms];
  int idx, entry;
  struct prng_rand_t state;
  //const TYPE infinity = (domainEdge*domainEdge*3.)*1000;//(max length)^2 * 1000

  // Create random positions in the box [0,domainEdge]^3
  prng_srand(1,&state);
  i=0;
  while( i<nAtoms ) {
    // Generate a new point
    p.x = domainEdge*(((TYPE)prng_rand(&state))/((TYPE)PRNG_RAND_MAX));
    p.y = domainEdge*(((TYPE)prng_rand(&state))/((TYPE)PRNG_RAND_MAX));
    p.z = domainEdge*(((TYPE)prng_rand(&state))/((TYPE)PRNG_RAND_MAX));
    // Assure that it's not directly on top of another atom
    reject = 0;
    for( idx=0; idx<nAtoms; idx++ ) {
      q = points[idx];
      if( dist_sq(p,q)<van_der_Waals_thresh ) {
        reject=1;
        break;
      }
    }
    if(!reject) {
      points[i] = p;
      //printf("%lf %lf %lf\n", points[i].x, points[i].y, points[i].z );
      ++i;
    }
  }

  // Insert points into blocks
  memset(data.n_points, 0, blockSide*blockSide*blockSide*sizeof(int32_t));
  memset(data.position, 0, 3*blockSide*blockSide*blockSide*densityFactor*sizeof(TYPE));
  for( idx=0; idx<nAtoms; idx++ ) {
    b.x = (int32_t) (points[idx].x / blockEdge);
    b.y = (int32_t) (points[idx].y / blockEdge);
    b.z = (int32_t) (points[idx].z / blockEdge);
    entry = data.n_points[b.x][b.y][b.z];
    data.position[b.x][b.y][b.z][entry].x = points[idx].x;
    data.position[b.x][b.y][b.z][entry].y = points[idx].y;
    data.position[b.x][b.y][b.z][entry].z = points[idx].z;
    ++data.n_points[b.x][b.y][b.z];
    assert(data.n_points[b.x][b.y][b.z]<densityFactor && "block overflow");
  }
  //for( b.x=0; b.x<blockSide; b.x++ ) {
  //for( b.y=0; b.y<blockSide; b.y++ ) {
  //for( b.z=0; b.z<blockSide; b.z++ ) {
  //  printf("grid[%d][%d][%d]: %d points\n", b.x, b.y, b.z, data.n_points[b.x][b.y][b.z]);
  //}}}

  // Open and write
  fd = open("input.data", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  assert( fd>0 && "Couldn't open input data file" );
  data_to_input(fd, (void *)(&data));

  return 0;
}
