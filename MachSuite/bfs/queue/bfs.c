/*
Implementation based on:
Hong, Oguntebi, Olukotun. "Efficient Parallel Graph Exploration on Multi-Core CPU and GPU." PACT, 2011.
*/

#include "bfs.h"

#ifdef DMA_MODE
#include "gem5/dma_interface.h"
#endif

#define Q_PUSH(node) { queue[q_in==0?N_NODES-1:q_in-1]=node; q_in=(q_in+1)%N_NODES; }
#define Q_PEEK() (queue[q_out])
#define Q_POP() { q_out = (q_out+1)%N_NODES; }
#define Q_EMPTY() (q_in>q_out ? q_in==q_out+1 : (q_in==0)&&(q_out==N_NODES-1))

void bfs(node_t nodes[N_NODES], edge_t edges[N_EDGES],
            node_index_t starting_node, level_t level[N_NODES],
            edge_index_t level_counts[N_LEVELS])
{
  node_index_t queue[N_NODES];
  node_index_t q_in, q_out;
  node_index_t dummy;
  node_index_t n;
  edge_index_t e;
  unsigned i;

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

  /*init_levels: for( n=0; n<N_NODES; n++ )*/
  /*level[n] = MAX_LEVEL;*/
  init_horizons: for( i=0; i<N_LEVELS; i++ )
    level_counts[i] = 0;

  q_in = 1;
  q_out = 0;
  level[starting_node] = 0;
  level_counts[0] = 1;
  Q_PUSH(starting_node);

  loop_queue: for( dummy=0; dummy<N_NODES; dummy++ ) { // Typically while(not_empty(queue)){
    if( Q_EMPTY() )
      break;
    n = Q_PEEK();
    Q_POP();
    edge_index_t tmp_begin = nodes[n].edge_begin;
    edge_index_t tmp_end = nodes[n].edge_end;
    loop_neighbors: for( e=tmp_begin; e<tmp_end; e++ ) {
      node_index_t tmp_dst = edges[e].dst;
      level_t tmp_level = level[tmp_dst];

      if( tmp_level ==MAX_LEVEL ) { // Unmarked
        level_t tmp_level = level[n]+1;
        level[tmp_dst] = tmp_level;
        ++level_counts[tmp_level];
        Q_PUSH(tmp_dst);
      }
    }
  }

  /*
  printf("Horizons:");
  for( i=0; i<N_LEVELS; i++ )
    printf(" %d", level_counts[i]);
  printf("\n");
  */
#ifdef DMA_MODE
  dmaStore(&level[0], 0, N_NODES * sizeof(level_t));
#endif
}
