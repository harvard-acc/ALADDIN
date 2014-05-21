#ifndef GRAPH_FUNC_H
#define GRAPH_FUNC_H

#include <igraph.h>
#include <iostream> 
#include <vector>
#include <assert.h>

using namespace std;
int check_edgeid(long int parent, long int updating_node, igraph_t *g);
bool is_edge_existed( int parent, int updating_node, igraph_t &g);
bool has_one_child(igraph_t *g, int node_id);
int isolated_nodes (igraph_t *g, unsigned num_of_vertices, vector<bool> &v_isolated);

void initialized_num_of_children(igraph_t *g, vector<int> &num_of_children);
unsigned int initialized_num_of_parents(igraph_t *g, vector<bool>
  vector_isolated, vector<int> &output);
#endif
