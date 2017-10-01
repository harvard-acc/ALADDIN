#include "base_address_init.h"

using namespace SrcTypes;

std::string BaseAddressInit::getCenteredName(size_t size) {
  return "       Init Base Address       ";
}

DynamicVariable BaseAddressInit::getCallerRegID(DynamicVariable& reg_id) {
  auto it = call_argument_map.find(reg_id);
  while (it != call_argument_map.end()) {
    reg_id = it->second;
    it = call_argument_map.find(reg_id);
  }
  return reg_id;
}

void BaseAddressInit::optimize() {
  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);

  vertex_iter vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(graph); vi != vi_end; ++vi) {
    if (boost::degree(*vi, graph) == 0)
      continue;
    Vertex curr_vertex = *vi;
    ExecNode* node = getNodeFromVertex(curr_vertex);
    if (!node->is_memory_op())
      continue;
    int node_microop = node->get_microop();
    // iterate its parents, until it finds the root parent
    while (true) {
      bool found_parent = false;
      in_edge_iter in_edge_it, in_edge_end;

      for (boost::tie(in_edge_it, in_edge_end) = in_edges(curr_vertex, graph);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        int edge_parid = edge_to_parid[*in_edge_it];
        /* For Load, not mem dependence. */
        /* For GEP, not the dependence that is caused by index. */
        /* For store, not the dependence caused by value or mem dependence. */
        if ((node_microop == LLVM_IR_Load && edge_parid != 1) ||
            (node_microop == LLVM_IR_GetElementPtr && edge_parid != 1) ||
            (node_microop == LLVM_IR_Store && edge_parid != 2))
          continue;

        unsigned parent_id = vertex_to_name[source(*in_edge_it, graph)];
        ExecNode* parent_node = exec_nodes.at(parent_id);
        int parent_microop = parent_node->get_microop();
        if (parent_microop == LLVM_IR_GetElementPtr ||
            parent_microop == LLVM_IR_Load || parent_microop == LLVM_IR_Store) {
          // remove address calculation directly
          DynamicVariable dynvar = parent_node->get_dynamic_variable();
          dynvar = getCallerRegID(dynvar);
          const std::string& label = dynvar.get_variable()->get_name();
          node->set_array_label(label);
          curr_vertex = source(*in_edge_it, graph);
          node_microop = parent_microop;
          found_parent = true;
          break;
        } else if (parent_microop == LLVM_IR_Alloca) {
          std::string label = exec_nodes.at(parent_id)->get_array_label();
          node->set_array_label(label);
          break;
        }
      }
      if (!found_parent)
        break;
    }
  }
}
