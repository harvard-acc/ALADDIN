#include "induction_dependence_removal.h"
#include "../DDDG.h"

// Induction dependence removal.
//
// This pass marks certain nodes as inductive. Inductive nodes are either nodes
// that only depend on induction variables or those for which all parents are
// inductive.
//
// For inductive nodes, depending on the opcode, we can also convert them to
// different opcodes as an additional optimization (for example, multiplication
// to shift).

using namespace SrcTypes;

std::string InductionDependenceRemoval::getCenteredName(size_t size) {
  return "  Remove Induction Dependence  ";
}

void InductionDependenceRemoval::optimize() {
  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);
  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    // Revert the inductive flag to false. This guarantees that no child node
    // of this node can mistakenly think its parent is inductive.
    node->set_inductive(false);
    if (!node->has_vertex() || node->is_memory_op())
      continue;
    Instruction* node_inst = node->get_static_inst();
    if (node_inst->is_inductive()) {
      node->set_inductive(true);
      if (node->is_int_add_op())
        node->set_microop(LLVM_IR_IndexAdd);
      else if (node->is_int_mul_op())
        node->set_microop(LLVM_IR_Shl);
    } else {
      /* If all the parents are inductive, then the current node is inductive.*/
      bool all_inductive = true;
      /* If one of the parent is inductive, and the operation is an integer mul,
       * do strength reduction that converts the mul/div to a shifter.*/
      bool any_inductive = false;
      in_edge_iter in_edge_it, in_edge_end;
      for (boost::tie(in_edge_it, in_edge_end) =
               in_edges(node->get_vertex(), graph);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        if (edge_to_parid[*in_edge_it] == CONTROL_EDGE)
          continue;
        Vertex parent_vertex = source(*in_edge_it, graph);
        ExecNode* parent_node = getNodeFromVertex(parent_vertex);
        if (!parent_node->is_inductive()) {
          all_inductive = false;
        } else {
          any_inductive = true;
        }
      }
      if (all_inductive) {
        node->set_inductive(true);
        if (node->is_int_add_op())
          node->set_microop(LLVM_IR_IndexAdd);
        else if (node->is_int_mul_op())
          node->set_microop(LLVM_IR_Shl);
      } else if (any_inductive) {
        if (node->is_int_mul_op())
          node->set_microop(LLVM_IR_Shl);
      }
    }
  }
}
