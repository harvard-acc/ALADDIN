#include "store_buffering.h"

std::string StoreBuffering::getCenteredName(size_t size) {
  return "          Store Buffer         ";
}

void StoreBuffering::optimize() {
  if (loop_bounds.size() <= 2)
    return;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);

  std::vector<NewEdge> to_add_edges;
  std::vector<unsigned> to_remove_nodes;

  auto bound_it = loop_bounds.begin();
  auto node_it = exec_nodes.begin();

  while (node_it != exec_nodes.end()) {
    while (node_it->first < bound_it->node_id && node_it != exec_nodes.end()) {
      ExecNode* node = node_it->second;
      if (!node->has_vertex() ||
          boost::degree(node->get_vertex(), graph) == 0) {
        ++node_it;
        continue;
      }
      if (node->is_store_op()) {
        // remove this store, unless it is a dynamic store which cannot be
        // statically disambiguated.
        if (node->is_dynamic_mem_op()) {
          ++node_it;
          continue;
        }
        Vertex node_vertex = node->get_vertex();
        out_edge_iter out_edge_it, out_edge_end;

        std::vector<Vertex> store_child;
        for (boost::tie(out_edge_it, out_edge_end) =
                 out_edges(node_vertex, graph);
             out_edge_it != out_edge_end;
             ++out_edge_it) {
          Vertex child_vertex = target(*out_edge_it, graph);
          ExecNode* child_node = getNodeFromVertex(child_vertex);
          if (child_node->is_load_op()) {
            if (child_node->is_dynamic_mem_op() ||
                child_node->get_node_id() >= (unsigned)bound_it->node_id)
              continue;
            else
              store_child.push_back(child_vertex);
          }
        }

        if (store_child.size() > 0) {
          bool parent_found = false;
          Vertex store_parent;
          in_edge_iter in_edge_it, in_edge_end;
          for (boost::tie(in_edge_it, in_edge_end) =
                   in_edges(node_vertex, graph);
               in_edge_it != in_edge_end;
               ++in_edge_it) {
            // parent node that generates value
            if (edge_to_parid[*in_edge_it] == 1) {
              parent_found = true;
              store_parent = source(*in_edge_it, graph);
              break;
            }
          }

          if (parent_found) {
            for (auto load_it = store_child.begin(), E = store_child.end();
                 load_it != E;
                 ++load_it) {
              Vertex load_node = *load_it;
              to_remove_nodes.push_back(vertex_to_name[load_node]);

              out_edge_iter out_edge_it, out_edge_end;
              for (boost::tie(out_edge_it, out_edge_end) =
                       out_edges(load_node, graph);
                   out_edge_it != out_edge_end;
                   ++out_edge_it) {
                Vertex target_vertex = target(*out_edge_it, graph);
                to_add_edges.push_back({ getNodeFromVertex(store_parent),
                                         getNodeFromVertex(target_vertex),
                                         edge_to_parid[*out_edge_it] });
              }
            }
          }
        }
      }
      ++node_it;
    }
    ++bound_it;
    if (bound_it == loop_bounds.end())
      break;
  }
  updateGraphWithNewEdges(to_add_edges);
  updateGraphWithIsolatedNodes(to_remove_nodes);
  cleanLeafNodes();
}
