#include "tree_height_reduction.h"

#include "../DDDG.h"

std::string TreeHeightReduction::getCenteredName(size_t size) {
  return "     Tree Height Reduction     ";
}

void TreeHeightReduction::optimize() {
  if (loop_bounds.size() <= 2)
    return;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);

  begin_node_id = exec_nodes.begin()->first;
  end_node_id = (--exec_nodes.end())->first + 1;

  std::map<unsigned, bool> updated;
  std::map<unsigned, int> bound_region;
  for (auto node_pair : exec_nodes) {
    updated[node_pair.first] = false;
    bound_region[node_pair.first] = 0;
  }

  int region_id = 0;
  auto node_it = exec_nodes.begin();
  unsigned node_id = node_it->first;
  auto bound_it = loop_bounds.begin();
  while (node_id <= bound_it->node_id && node_it != exec_nodes.end()) {
    bound_region.at(node_id) = region_id;
    if (node_id == bound_it->node_id) {
      region_id++;
      bound_it++;
      if (bound_it == loop_bounds.end())
        break;
    }
    node_it++;
    node_id = node_it->first;
  }

  std::set<Edge> to_remove_edges;
  std::vector<NewEdge> to_add_edges;

  // nodes with no outgoing edges to first (bottom nodes first)
  for (auto node_it = exec_nodes.rbegin(); node_it != exec_nodes.rend();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (!node->has_vertex() || boost::degree(node->get_vertex(), graph) == 0 ||
        updated.at(node->get_node_id()) || !node->is_associative())
      continue;
    unsigned node_id = node->get_node_id();
    updated.at(node_id) = 1;
    int node_region = bound_region.at(node_id);

    std::list<ExecNode*> nodes;
    std::vector<Edge> tmp_remove_edges;
    std::vector<std::pair<ExecNode*, bool>> leaves;
    std::vector<ExecNode*> associative_chain;
    associative_chain.push_back(node);
    unsigned chain_id = 0;
    while (chain_id < associative_chain.size()) {
      ExecNode* chain_node = associative_chain[chain_id];
      if (chain_node->is_associative()) {
        updated.at(chain_node->get_node_id()) = 1;
        int num_of_chain_parents = 0;
        in_edge_iter in_edge_it, in_edge_end;
        for (boost::tie(in_edge_it, in_edge_end) =
                 in_edges(chain_node->get_vertex(), graph);
             in_edge_it != in_edge_end;
             ++in_edge_it) {
          if (edge_to_parid[*in_edge_it] == CONTROL_EDGE)
            continue;
          num_of_chain_parents++;
        }
        if (num_of_chain_parents == 2) {
          nodes.push_front(chain_node);
          for (boost::tie(in_edge_it, in_edge_end) =
                   in_edges(chain_node->get_vertex(), graph);
               in_edge_it != in_edge_end;
               ++in_edge_it) {
            if (edge_to_parid[*in_edge_it] == CONTROL_EDGE)
              continue;
            Vertex parent_vertex = source(*in_edge_it, graph);
            int parent_id = vertex_to_name[parent_vertex];
            ExecNode* parent_node = getNodeFromVertex(parent_vertex);
            assert(*parent_node < *chain_node);

            Edge curr_edge = *in_edge_it;
            tmp_remove_edges.push_back(curr_edge);
            int parent_region = bound_region.at(parent_id);
            if (parent_region == node_region) {
              updated.at(parent_id) = 1;
              if (!parent_node->is_associative())
                leaves.push_back(std::make_pair(parent_node, false));
              else {
                out_edge_iter out_edge_it, out_edge_end;
                int num_of_children = 0;
                for (boost::tie(out_edge_it, out_edge_end) =
                         out_edges(parent_vertex, graph);
                     out_edge_it != out_edge_end;
                     ++out_edge_it) {
                  if (edge_to_parid[*out_edge_it] != CONTROL_EDGE)
                    num_of_children++;
                }

                if (num_of_children == 1)
                  associative_chain.push_back(parent_node);
                else
                  leaves.push_back(std::make_pair(parent_node, false));
              }
            } else {
              leaves.push_back(std::make_pair(parent_node, true));
            }
          }
        } else {
          /* Promote the single parent node with higher priority. This affects
           * mostly the top of the graph where no parent exists. */
          leaves.push_back(std::make_pair(chain_node, true));
        }
      } else {
        leaves.push_back(std::make_pair(chain_node, false));
      }
      chain_id++;
    }
    // build the tree
    if (nodes.size() < 3)
      continue;
    for (auto it = tmp_remove_edges.begin(), E = tmp_remove_edges.end();
         it != E;
         it++)
      to_remove_edges.insert(*it);

    std::map<ExecNode*, unsigned> rank_map;
    auto leaf_it = leaves.begin();

    while (leaf_it != leaves.end()) {
      if (leaf_it->second == false)
        rank_map[leaf_it->first] = begin_node_id;
      else
        rank_map[leaf_it->first] = end_node_id;
      ++leaf_it;
    }
    // reconstruct the rest of the balanced tree
    // TODO: Find a better name for this.
    auto new_node_it = nodes.begin();
    while (new_node_it != nodes.end()) {
      ExecNode* node1 = nullptr;
      ExecNode* node2 = nullptr;
      if (rank_map.size() == 2) {
        node1 = rank_map.begin()->first;
        node2 = (++rank_map.begin())->first;
      } else {
        findMinRankNodes(&node1, &node2, rank_map);
      }
      // TODO: Is this at all possible...?
      assert((node1->get_node_id() != end_node_id) &&
             (node2->get_node_id() != end_node_id));
      to_add_edges.push_back({ node1, *new_node_it, 1 });
      to_add_edges.push_back({ node2, *new_node_it, 1 });

      // place the new node in the map, remove the two old nodes
      rank_map[*new_node_it] = std::max(rank_map[node1], rank_map[node2]) + 1;
      rank_map.erase(node1);
      rank_map.erase(node2);
      ++new_node_it;
    }
  }
  /*For tree reduction, we always remove edges first, then add edges. The reason
   * is that it's possible that we are adding the same edges as the edges that
   * we want to remove. Doing adding edges first then removing edges could lead
   * to losing dependences.*/
  updateGraphWithIsolatedEdges(to_remove_edges);
  updateGraphWithNewEdges(to_add_edges);
  cleanLeafNodes();
}

void TreeHeightReduction::findMinRankNodes(
    ExecNode** node1,
    ExecNode** node2,
    std::map<ExecNode*, unsigned>& rank_map) {
  unsigned min_rank = end_node_id;
  for (auto it = rank_map.begin(); it != rank_map.end(); ++it) {
    unsigned node_rank = it->second;
    if (node_rank < min_rank) {
      *node1 = it->first;
      min_rank = node_rank;
    }
  }
  min_rank = end_node_id;
  for (auto it = rank_map.begin(); it != rank_map.end(); ++it) {
    unsigned node_rank = it->second;
    if ((it->first != *node1) && (node_rank < min_rank)) {
      *node2 = it->first;
      min_rank = node_rank;
    }
  }
}
