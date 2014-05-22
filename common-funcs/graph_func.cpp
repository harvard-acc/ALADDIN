#include "graph_func.h"


/*Return the Edge ID
Input: source node id and sink node id, igraph_t
Output: Edge ID
*/
int check_edgeid(long int parent, long int updating_node, igraph_t *g)
{
	igraph_es_t		es_edgetype;
	igraph_eit_t	eit_edgetype;
	igraph_es_pairs_small(&es_edgetype, IGRAPH_DIRECTED, parent, updating_node, -1);
	igraph_eit_create(g, es_edgetype, &eit_edgetype);
  assert(IGRAPH_EIT_SIZE(eit_edgetype) == 1);
  int edgeid = (int)IGRAPH_EIT_GET(eit_edgetype);
  igraph_es_destroy(&es_edgetype);
  igraph_eit_destroy(&eit_edgetype);
	return  edgeid;
}

bool is_edge_existed( int parent, int updating_node, igraph_t &g)
{
  igraph_vs_t vs_children;
  igraph_vit_t vit_children;
  igraph_vs_adj(&vs_children, parent, IGRAPH_OUT);
  igraph_vit_create(&g, vs_children, &vit_children);

  if (IGRAPH_VIT_SIZE(vit_children) == 0)
    return false;
  while(!IGRAPH_VIT_END(vit_children))
  {
    if (IGRAPH_VIT_GET(vit_children) == updating_node)
      return true;
    IGRAPH_VIT_NEXT(vit_children);
  }
  return false;
}

bool has_one_child(igraph_t *g, int node_id)
{
  igraph_vs_t vs_children;
  igraph_vit_t vit_children;
  igraph_vs_adj(&vs_children, node_id, IGRAPH_OUT);
  igraph_vit_create(g, vs_children, &vit_children);
  if (IGRAPH_VIT_SIZE(vit_children) == 1)
  {
    igraph_vs_destroy(&vs_children);
    igraph_vit_destroy(&vit_children);
    return true;
  }
  igraph_vs_destroy(&vs_children);
  igraph_vit_destroy(&vit_children);
  return false;
}

/*Check the number of children each node has
save to the output array
return the total number of connected nodes*/
void initialized_num_of_children(igraph_t *g, vector<int> &num_of_children)
{
  unsigned num_of_vertices = (unsigned) igraph_vcount(g);
  for (unsigned int i = 0; i < num_of_vertices; ++i)
  {
    igraph_vs_t vs_children;
    igraph_vit_t vit_children;
    igraph_vs_adj(&vs_children, i, IGRAPH_OUT);
    igraph_vit_create(g, vs_children, &vit_children);
    num_of_children[i] = IGRAPH_VIT_SIZE(vit_children);
    igraph_vs_destroy(&vs_children);
    igraph_vit_destroy(&vit_children);
  }
}

/*Check the number of parents each node has
save to the output array
return the total number of connected nodes*/
unsigned int initialized_num_of_parents(igraph_t *g, vector<bool>
v_isolated, vector<int> &num_of_parents)
{
#ifdef DEBUG
  std::cerr << "in initialized_num_of_parents" << endl;
#endif
  unsigned num_of_vertices = (unsigned) igraph_vcount(g);
  unsigned total_connected_nodes = 0;
  for (unsigned int i = 0; i < num_of_vertices; ++i)
  {
    if (v_isolated[i] == 1)
    {
      continue;
    }
    else
    { 
      total_connected_nodes++;
      igraph_vs_t vs_parents;
      igraph_vit_t vit_parents;
      igraph_vs_adj(&vs_parents, i, IGRAPH_IN);
      igraph_vit_create(g, vs_parents, &vit_parents);
      num_of_parents[i] = IGRAPH_VIT_SIZE(vit_parents);
      igraph_vs_destroy(&vs_parents);
      igraph_vit_destroy(&vit_parents);
    }
  }
#ifdef DEBUG
  std::cerr << "leaving initialized_num_of_parents" << endl;
#endif
  return total_connected_nodes;
}

int isolated_nodes (igraph_t *g, unsigned num_of_vertices, vector<bool> &v_isolated)
{
#ifdef DEBUG
  std::cerr << "in isolate_nodes" << endl;
#endif
  int isolated = 0;
  for (unsigned int node_id = 0; node_id < num_of_vertices; node_id++)
  {
    igraph_vs_t 		vs_parent;
    igraph_vit_t 		vit_parent_it;
    igraph_vs_adj(&vs_parent, node_id, IGRAPH_IN);	
    igraph_vit_create(g, vs_parent, &vit_parent_it);
    
    igraph_vs_t 		vs_child;
    igraph_vit_t 		vit_child_it;
    igraph_vs_adj(&vs_child, node_id, IGRAPH_OUT);	
    igraph_vit_create(g, vs_child, &vit_child_it);
    
    if (IGRAPH_VIT_SIZE(vit_parent_it) == 0)
      if (IGRAPH_VIT_SIZE(vit_child_it) == 0)
      {
        v_isolated.at(node_id) = 1;
        isolated++;
      }
    
    igraph_vs_destroy(&vs_child);
    igraph_vit_destroy(&vit_child_it);
    igraph_vs_destroy(&vs_parent);
    igraph_vit_destroy(&vit_parent_it);
  }
#ifdef DEBUG
  std::cerr << "leaving  isolate_nodes" << endl;
#endif
  return isolated;
}
