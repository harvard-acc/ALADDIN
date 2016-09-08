#include "md.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void md(TYPE d_force_x[nAtoms], TYPE d_force_y[nAtoms], TYPE d_force_z[nAtoms],
      TYPE position_x[nAtoms], TYPE position_y[nAtoms], TYPE position_z[nAtoms],
      TYPE NL[32*32]) {
#ifdef DMA_MODE
	dmaLoad(&d_force_x[0], 0, nAtoms*sizeof(TYPE));
	dmaLoad(&d_force_y[0], 0, nAtoms*sizeof(TYPE));
	dmaLoad(&d_force_z[0], 0, nAtoms*sizeof(TYPE));
	dmaLoad(&position_x[0], 0, nAtoms*sizeof(TYPE));
	dmaLoad(&position_y[0], 0, nAtoms*sizeof(TYPE));
	dmaLoad(&position_z[0], 0, nAtoms*sizeof(TYPE));
	dmaLoad(&NL[0], 0, 32*32*sizeof(TYPE));
#endif
	int i, j, jidx;
	TYPE delx, dely, delz, r2inv, r2invTEMP, r2invTEMP2, r2invTEMP3, t1, t2, t3;
	loop_i : for (i = 0; i < nAtoms; i++)
  {
    TYPE i_x = position_x[i];
    TYPE i_y = position_y[i];
    TYPE i_z = position_z[i];
    TYPE fx = 0;
    TYPE fy = 0;
    TYPE fz = 0;
    loop_j : for( j = 0; j < maxNeighbors; j++)
    {
      jidx = NL[i*32 + j];
      TYPE j_x = position_x[jidx];
      TYPE j_y = position_y[jidx];
      TYPE j_z = position_z[jidx];
      delx = i_x - j_x;
      dely = i_y - j_y;
      delz = i_z - j_z;
      r2invTEMP = delx * delx;
      r2invTEMP2 = dely * dely;
      r2invTEMP3 = delz * delz;
      t1 = r2invTEMP + r2invTEMP2;
      t2 = t1 + r2invTEMP3;
      r2inv = t2;
      TYPE r6inv = r2inv * r2inv * r2inv;
      TYPE force = r2inv*r6inv*(lj1*r6inv - lj2);
      fx += delx * force;
      fy += dely * force;
      fz += delz * force;
    }
    d_force_x[i] = fx;
    d_force_y[i] = fy;
    d_force_z[i] = fz;
  }
#ifdef DMA_MODE
	dmaStore(&d_force_x[0], 0, nAtoms*sizeof(TYPE));
	dmaStore(&d_force_y[0], 0, nAtoms*sizeof(TYPE));
	dmaStore(&d_force_z[0], 0, nAtoms*sizeof(TYPE));
#endif
}

TYPE distance(
		TYPE position_x[nAtoms],
		TYPE position_y[nAtoms],
		TYPE position_z[nAtoms],
		int i,
		int j)
{
	TYPE delx, dely, delz, r2inv;
	delx = position_x[i] - position_x[j];
	dely = position_y[i] - position_y[j];
	delz = position_z[i] - position_z[j];
	r2inv = delx * delx + dely * dely + delz * delz;
	return r2inv;
}

inline void insertInOrder(TYPE currDist[maxNeighbors],
		int currList[maxNeighbors],
		int j,
		TYPE distIJ)
{
	int dist, pos, currList_t;
	TYPE currMax, currDist_t;
	pos = maxNeighbors - 1;
	currMax = currDist[pos];
	if (distIJ > currMax){
		return;
	}
	for (dist = pos; dist > 0; dist--){
		if (distIJ < currDist[dist]){
			currDist[dist] = currDist[dist - 1];
			currList[dist]  = currList[pos  - 1];
		}
		else{
			break;
		}
		pos--;
	}
	currDist[dist] = distIJ;
	currList[dist]  = j;
}

int buildNeighborList(TYPE position_x[nAtoms],
		TYPE position_y[nAtoms],
		TYPE position_z[nAtoms],
		int NL[nAtoms][maxNeighbors]
		)

{
	int totalPairs, i, j, k;
	totalPairs = 0;
	TYPE distIJ;
	for (i = 0; i < nAtoms; i++){
		int currList[maxNeighbors];
		TYPE currDist[maxNeighbors];
		for(k=0; k<maxNeighbors; k++){
			currList[k] = 0;
			currDist[k] = 999999999;
		}
		for (j = 0; j < nAtoms; j++){
			if (i == j){
				continue;
			}
			distIJ = distance(position_x, position_y, position_z, i, j);
			//insertInOrder(currDist, currList, j, distIJ, maxNeighbors);
			currList[j] = j;
			currDist[j] = distIJ;
		}
		totalPairs += populateNeighborList(currDist, currList, i, NL);
	}
	printf("total pairs - %i \n", totalPairs);
	return totalPairs;
}

int populateNeighborList(TYPE currDist[maxNeighbors],
		int currList[maxNeighbors],
		const int i,
		int NL[nAtoms][maxNeighbors])
{
	int idx, validPairs, distanceIter, neighborIter;
	idx = 0; validPairs = 0;
	//distanceIter = 0;
	for (neighborIter = 0; neighborIter < maxNeighbors; neighborIter++){
		//if(currList[neighborIter] == -1){
		//	break;
		//}
		NL[i][neighborIter] = currList[neighborIter];
		//if (currDist[distanceIter] < cutsq)
		validPairs++;
		//idx++;
		//distanceIter++;
	}
	return validPairs;
}

int main(){
	int sizeClass, passes, iter, nAtom, err, localSize, globalSize, i, j, totalPairs;
	int probSizes[4] = {12288, 24576, 36864, 73728};
	sizeClass = 1; passes = 1; iter = 1; err = 0; localSize= 128; globalSize = nAtoms;
	srand(8650341L);
	//instantiate...
	TYPE position_x[nAtoms];
	TYPE position_y[nAtoms];
	TYPE position_z[nAtoms];
	TYPE force_x[nAtoms];
	TYPE force_y[nAtoms];
	TYPE force_z[nAtoms];
	TYPE NL[nAtoms][maxNeighbors];
  int neighborList[32*32];
	//initialize positions
	for  (i = 0; i < nAtoms; i++)
	{
		position_x[i] = rand();
		position_y[i] = rand();
		position_z[i] = rand();
    /*position_x[i] = (TYPE)((TYPE)rand() / (TYPE)(RAND_MAX * domainEdge));*/
    /*position_y[i] = (TYPE)((TYPE)rand() / (TYPE)(RAND_MAX * domainEdge));*/
    /*position_z[i] = (TYPE)((TYPE)rand() / (TYPE)(RAND_MAX * domainEdge));*/
	}
	//zero out NL...
	for(i=0; i<nAtoms; i++){
		for(j = 0; j < maxNeighbors; ++j){
			NL[i][j] = 0;
		}
	}
	//Compute the neighbors...
	totalPairs = buildNeighborList(position_x, position_y, position_z, NL);
	//Check what we got...
  /*for(i=0; i<nAtoms; i++){*/
  /*for(j = 0; j < maxNeighbors; ++j){*/
  /*printf("NL[%i][%i] = %i.. \n", i, j, NL[i][j]);*/
  /*}*/
  /*}*/
	//Call MD accelerator
  for(i=0; i<32; i++){
          for(j = 0; j < 32; ++j)
            neighborList[i*32 + j] = NL[i][j];
  }
#ifdef GEM5
  resetGem5Stats();
#endif
	md(force_x, force_y, force_z, position_x, position_y, position_z, neighborList);
#ifdef GEM5
  dumpGem5Stats("md");
#endif
	//print positions.. doesn't mean shit though..
	for(i=0; i<nAtoms; i++){
		printf("after, X:%i Y:%i Z%i \n", force_x[i], force_y[i], force_z[i]);
	}
	//return 0 for pass..
	return 0;
}
