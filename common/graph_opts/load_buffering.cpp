#include "load_buffering.h"

std::string LoadBuffering::getCenteredName(size_t size) {
  return "          Load Buffer          ";
}

void LoadBuffering::optimize() {
  if (!user_params.unrolling.size() && loop_bounds.size() <= 2)
    return;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);

  vertex_iter vi, vi_end;

  std::set<Edge> to_remove_edges;
  std::vector<NewEdge> to_add_edges;

  int shared_loads = 0;
  auto bound_it = loop_bounds.begin();

  auto node_it = exec_nodes.begin();
  while (node_it != exec_nodes.end()) {
    std::unordered_map<unsigned, ExecNode*> address_loaded;
    while (node_it->first < bound_it->node_id && node_it != exec_nodes.end()) {
      ExecNode* node = node_it->second;
      if (!node->has_vertex() ||
          boost::degree(node->get_vertex(), graph) == 0 ||
          !node->is_memory_op()) {
        node_it++;
        continue;
      }
      Addr node_address = node->get_mem_access()->vaddr;
      auto addr_it = address_loaded.find(node_address);
      if (node->is_store_op() && addr_it != address_loaded.end()) {
        address_loaded.erase(addr_it);
      } else if (node->is_load_op()) {
        if (addr_it == address_loaded.end()) {
          address_loaded[node_address] = node;
        } else {
          // check whether the current load is dynamic or not.
          if (node->is_dynamic_mem_op()) {
            ++node_it;
            continue;
          }
          shared_loads++;
          node->set_microop(LLVM_IR_Move);
          ExecNode* prev_load = addr_it->second;
          // iterate through its children
          Vertex load_node = node->get_vertex();
          out_edge_iter out_edge_it, out_edge_end;
          for (boost::tie(out_edge_it, out_edge_end) =
                   out_edges(load_node, graph);
               out_edge_it != out_edge_end;
               ++out_edge_it) {
            Edge curr_edge = *out_edge_it;
            Vertex child_vertex = target(curr_edge, graph);
            assert(prev_load->has_vertex());
            Vertex prev_load_vertex = prev_load->get_vertex();
            if (!doesEdgeExist(prev_load_vertex, child_vertex)) {
              ExecNode* child_node = getNodeFromVertex(child_vertex);
              to_add_edges.push_back(
                  { prev_load, child_node, edge_to_parid[curr_edge] });
            }
            to_remove_edges.insert(*out_edge_it);
          }
          in_edge_iter in_edge_it, in_edge_end;
          for (boost::tie(in_edge_it, in_edge_end) =
                   in_edges(load_node, graph);
               in_edge_it != in_edge_end;
               ++in_edge_it)
            to_remove_edges.insert(*in_edge_it);
        }
      }
      node_it++;
    }
    bound_it++;
    if (bound_it == loop_bounds.end())
      break;
  }
  updateGraphWithNewEdges(to_add_edges);
  updateGraphWithIsolatedEdges(to_remove_edges);
  cleanLeafNodes();
}
