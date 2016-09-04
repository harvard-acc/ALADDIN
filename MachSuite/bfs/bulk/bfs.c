/*
Implementations based on:
Harish and Narayanan. "Accelerating large graph algorithms on the GPU using CUDA." HiPC, 2007.
Hong, Oguntebi, Olukotun. "Efficient Parallel Graph Exploration on Multi-Core CPU and GPU." PACT, 2011.
*/

#include "bfs.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void bfs(node_t nodes[N_NODES], edge_t edges[N_EDGES],
            node_index_t starting_node, level_t level[N_NODES],
            edge_index_t level_counts[N_LEVELS])
{
  node_index_t n;
  edge_index_t e;
  level_t horizon;
  edge_index_t cnt;

#ifdef DMA_MODE
  dmaLoad(&level[0], 0, N_NODES * sizeof(level_t));
  dmaLoad(&nodes[0], 0, N_NODES * sizeof(node_t));
  dmaLoad(&edges[0], 0 * 512 * sizeof(edge_t), PAGE_SIZE);
  dmaLoad(&edges[0], 1 * 512 * sizeof(edge_t), PAGE_SIZE);
  dmaLoad(&edges[0], 2 * 512 * sizeof(edge_t), PAGE_SIZE);
  dmaLoad(&edges[0], 3 * 512 * sizeof(edge_t), PAGE_SIZE);
  dmaLoad(&edges[0], 4 * 512 * sizeof(edge_t), PAGE_SIZE);
  dmaLoad(&edges[0], 5 * 512 * sizeof(edge_t), PAGE_SIZE);
  dmaLoad(&edges[0], 6 * 512 * sizeof(edge_t), PAGE_SIZE);
  dmaLoad(&edges[0], 7 * 512 * sizeof(edge_t), PAGE_SIZE);
#endif

  level[starting_node] = 0;
  level_counts[0] = 1;

  loop_horizons: for( horizon=0; horizon<N_LEVELS; horizon++ ) {
    cnt = 0;
    // Add unmarked neighbors of the current horizon to the next horizon
    loop_nodes: for( n=0; n<N_NODES; n++ ) {
      if( level[n]==horizon ) {
        edge_index_t tmp_begin = nodes[n].edge_begin;
        edge_index_t tmp_end = nodes[n].edge_end;
        loop_neighbors: for( e=tmp_begin; e<tmp_end; e++ ) {
          node_index_t tmp_dst = edges[e].dst;
          level_t tmp_level = level[tmp_dst];

          if( tmp_level ==MAX_LEVEL ) { // Unmarked
            level[tmp_dst] = horizon+1;
            ++cnt;
          }
        }
      }
    }
    if( (level_counts[horizon+1]=cnt)==0 )
      break;
  }
#ifdef DMA_MODE
  dmaStore(&level[0], 0, N_NODES * sizeof(level_t));
#endif
}
