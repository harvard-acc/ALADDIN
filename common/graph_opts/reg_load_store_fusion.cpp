#include "reg_load_store_fusion.h"

#include "../DDDG.h"

// Register load-store fusion.
//
// A value stored in a register does not need to take one cycle for a "Load"
// before it can be used. Similarly, a value being written to a register does
// not require an entire cycle, as long as whatever is producing the value
// completes before the required setup time of the clock edge. But LLVM will
// emit load and store IR instructions for all arrays as it does not know
// that some arrays are not actually SRAM structures. For completely partitioned
// arrays, convert the memory dependence edge between loads, the consuming
// node, and the store into a special register dependence edge. This tells
// the scheduler to schedule the two nodes on the same cycle.

std::string RegLoadStoreFusion::getCenteredName(size_t size) {
  return "  Fuse register loads/stores   ";
}

void RegLoadStoreFusion::optimize() {

  std::vector<NewEdge> to_add_edges;
  std::set<Edge> to_remove_edges;
  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);

  for (auto& node_pair : exec_nodes) {
    ExecNode* node = node_pair.second;
    if (!node->is_load_op() && !node->is_store_op())
      continue;
    auto part_it = user_params.partition.find(node->get_array_label());
    if (part_it == user_params.partition.end() ||
        part_it->second.partition_type != complete)
      continue;

    if (node->is_load_op()) {
      Vertex load_vertex = node->get_vertex();
      out_edge_iter out_edge_it, out_edge_end;
      for (boost::tie(out_edge_it, out_edge_end) = out_edges(load_vertex, graph);
           out_edge_it != out_edge_end;
           ++out_edge_it) {
        if (edge_to_parid[*out_edge_it] == CONTROL_EDGE)
          continue;

        Vertex target_vertex = target(*out_edge_it, graph);
        ExecNode* target_node = getNodeFromVertex(target_vertex);
        // A register load cannot be fused with another load operation.
        if (target_node->is_load_op())
          continue;

        to_remove_edges.insert(*out_edge_it);
        to_add_edges.push_back({ node, target_node, REGISTER_EDGE });
      }
    } else if (node->is_store_op()) {
      Vertex store_vertex = node->get_vertex();
      in_edge_iter in_edge_it, in_edge_end;
      for (boost::tie(in_edge_it, in_edge_end) = in_edges(store_vertex, graph);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        if (edge_to_parid[*in_edge_it] == CONTROL_EDGE)
          continue;

        Vertex source_vertex = source(*in_edge_it, graph);
        ExecNode* source_node = getNodeFromVertex(source_vertex);
        // A register store cannot be fused with another store operation.
        if (source_node->is_store_op())
          continue;

        to_remove_edges.insert(*in_edge_it);
        to_add_edges.push_back({ source_node, node, REGISTER_EDGE });
      }
    }
  }

  updateGraphWithIsolatedEdges(to_remove_edges);
  updateGraphWithNewEdges(to_add_edges);
  cleanLeafNodes();
}
