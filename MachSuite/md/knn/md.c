/*
Implemenataion based on:
A. Danalis, G. Marin, C. McCurdy, J. S. Meredith, P. C. Roth, K. Spafford, V. Tipparaju, and J. S. Vetter.
The scalable heterogeneous computing (shoc) benchmark suite.
In Proceedings of the 3rd Workshop on General-Purpose Computation on Graphics Processing Units, 2010.
*/

#include "md.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void md_kernel(TYPE* host_force_x,
               TYPE* host_force_y,
               TYPE* host_force_z,
               TYPE* host_position_x,
               TYPE* host_position_y,
               TYPE* host_position_z,
               int32_t* host_NL,
               TYPE* force_x,
               TYPE* force_y,
               TYPE* force_z,
               TYPE* position_x,
               TYPE* position_y,
               TYPE* position_z,
               int32_t* NL)

{
    TYPE delx, dely, delz, r2inv;
    TYPE r6inv, potential, force, j_x, j_y, j_z;
    TYPE i_x, i_y, i_z, fx, fy, fz;
    int32_t i, j, jidx;

#ifdef DMA_MODE
    dmaLoad(position_x, host_position_x, nAtoms * sizeof(TYPE));
    dmaLoad(position_y, host_position_y, nAtoms * sizeof(TYPE));
    dmaLoad(position_z, host_position_z, nAtoms * sizeof(TYPE));
    dmaLoad(NL, host_NL, nAtoms * maxNeighbors * sizeof(int32_t));
#else
    position_x = host_position_x;
    position_y = host_position_y;
    position_z = host_position_z;
    NL = host_NL;
    force_x = host_force_x;
    force_y = host_force_y;
    force_z = host_force_z;
#endif

loop_i : for (i = 0; i < nAtoms; i++){
             i_x = position_x[i];
             i_y = position_y[i];
             i_z = position_z[i];
             fx = 0;
             fy = 0;
             fz = 0;
loop_j : for( j = 0; j < maxNeighbors; j++){
             // Get neighbor
             jidx = NL[i*maxNeighbors + j];
             // Look up x,y,z positions
             j_x = position_x[jidx];
             j_y = position_y[jidx];
             j_z = position_z[jidx];
             // Calc distance
             delx = i_x - j_x;
             dely = i_y - j_y;
             delz = i_z - j_z;
             r2inv = 1.0/( delx*delx + dely*dely + delz*delz );
             // Assume no cutoff and aways account for all nodes in area
             r6inv = r2inv * r2inv * r2inv;
             potential = r6inv*(lj1*r6inv - lj2);
             // Sum changes in force
             force = r2inv*potential;
             fx += delx * force;
             fy += dely * force;
             fz += delz * force;
         }
         //Update forces after all neighbors accounted for.
         force_x[i] = fx;
         force_y[i] = fy;
         force_z[i] = fz;
         //printf("dF=%lf,%lf,%lf\n", fx, fy, fz);
         }
#ifdef DMA_MODE
    dmaStore(host_force_x, force_x, nAtoms * sizeof(TYPE));
    dmaStore(host_force_y, force_y, nAtoms * sizeof(TYPE));
    dmaStore(host_force_z, force_z, nAtoms * sizeof(TYPE));
#endif
}
