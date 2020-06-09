/*
Implementations based on:
Harish and Narayanan. "Accelerating large graph algorithms on the GPU using CUDA." HiPC, 2007.
Hong, Oguntebi, Olukotun. "Efficient Parallel Graph Exploration on Multi-Core CPU and GPU." PACT, 2011.
*/

#include "bfs.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

void bfs(node_t* host_nodes,
         edge_t* host_edges,
         level_t* host_level,
         edge_index_t* host_level_counts,
         node_t* nodes,
         edge_t* edges,
         level_t* level,
         edge_index_t* level_counts,
         node_index_t starting_node)
{
  node_index_t n;
  edge_index_t e;
  level_t horizon;
  edge_index_t cnt;
  int i;

#ifdef DMA_MODE
  dmaLoad(nodes, host_nodes, N_NODES * sizeof(node_t));
  dmaLoad(edges, host_edges, N_EDGES * sizeof(edge_t));
  dmaLoad(level, host_level, N_NODES * sizeof(level_t));
#else
  nodes = host_nodes;
  edges = host_edges;
  level = host_level;
  level_counts = host_level_counts;
#endif

  level[starting_node] = 0;
  init_horizons: for( i=0; i<N_LEVELS; i++ )
    level_counts[i] = 0;
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
  dmaStore(host_level_counts, level_counts, N_LEVELS * sizeof(edge_index_t));
#endif
}
