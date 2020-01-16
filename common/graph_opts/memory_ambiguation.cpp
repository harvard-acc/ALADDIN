#include <list>
#include <vector>

#include "memory_ambiguation.h"

#include "../DDDG.h"
#include "../DynamicEntity.h"

// Memory ambiguation.
//
// Aladdin has perfect knowledge of all memory addresses, so all memory
// dependences are known to be true dependences. But there are some
// memory addresses that cannot be determined statically; these are true
// data-dependent memory accesses. This pass identifies such memory operations,
// marks them as dynamic operations, and in certain cases, will serialize them.

using namespace SrcTypes;

std::string MemoryAmbiguationOpt::getCenteredName(size_t size) {
  return "      Memory Ambiguation       ";
}

void MemoryAmbiguationOpt::optimize() {
  std::vector<NewEdge> to_add_edges;
  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);
  using gep_store_pair_t = std::pair<ExecNode*, ExecNode*>;
  std::unordered_map<DynamicInstruction, std::vector<gep_store_pair_t>>
      possible_dependent_stores;
  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (!node->is_memory_op() || !node->has_vertex())
      continue;
    in_edge_iter in_edge_it, in_edge_end;
    for (boost::tie(in_edge_it, in_edge_end) =
             in_edges(node->get_vertex(), graph);
         in_edge_it != in_edge_end;
         ++in_edge_it) {
      Vertex parent_vertex = source(*in_edge_it, graph);
      ExecNode* parent_node = getNodeFromVertex(parent_vertex);
      if (parent_node->get_microop() != LLVM_IR_GetElementPtr)
        continue;
      if (!parent_node->is_inductive()) {
        node->set_dynamic_mem_op(true);
        if (node->is_store_op()) {
          DynamicInstruction unique_id = node->get_dynamic_instruction();
          if (possible_dependent_stores.find(unique_id) ==
              possible_dependent_stores.end())
            possible_dependent_stores[unique_id] =
                std::vector<gep_store_pair_t>();

          possible_dependent_stores[unique_id].push_back(
              std::make_pair(parent_node, node));
        }
      }
    }
  }

  // For each dynamic instruction that may potentially produce dependent
  // stores, check if the ops differ in their addresses by only an induction
  // variable. If so, then they are still dynamic memory ops but do not need to
  // be serialized.
  std::vector<DynamicInstruction> stores_to_serialize;
  for (const auto& kv_pair : possible_dependent_stores) {
    const std::vector<gep_store_pair_t>& store_list = kv_pair.second;
    std::list<MemoryAddrSources> all_sources;
    for (const gep_store_pair_t& pair : store_list) {
      ExecNode* gep = pair.first;
      all_sources.push_back(findMemoryAddrSources(
          gep, gep->get_static_function(), edge_to_parid));
    }
    unsigned idx = 0;
    for (auto it = all_sources.begin(); it != --all_sources.end(); ++idx) {
      MemoryAddrSources& curr_source = *it;
      MemoryAddrSources& next_source = *(++it);
      if (!curr_source.is_independent_of(next_source, idx == 0)) {
        to_add_edges.push_back({ store_list[idx].second,
                                 store_list[idx + 1].second, MEMORY_EDGE });
      }
    }
  }

  updateGraphWithNewEdges(to_add_edges);
}

MemoryAddrSources MemoryAmbiguationOpt::findMemoryAddrSources(
    ExecNode* current_node,
    Function* current_func,
    EdgeNameMap& edge_to_parid) {
  in_edge_iter in_edge_it, in_edge_end;
  MemoryAddrSources sources;
  for (boost::tie(in_edge_it, in_edge_end) =
           in_edges(current_node->get_vertex(), graph);
       in_edge_it != in_edge_end;
       ++in_edge_it) {
    if (edge_to_parid[*in_edge_it] == CONTROL_EDGE)
      continue;
    Vertex source_vertex = source(*in_edge_it, graph);
    ExecNode* parent_node = getNodeFromVertex(source_vertex);
    if (parent_node->is_load_op()) {
      sources.add_noninductive(parent_node);
      continue;
    }
    if ((parent_node->is_gep_op() || parent_node->is_compute_op()) &&
        parent_node->get_static_function() == current_func) {
      if (parent_node->get_microop() == LLVM_IR_IndexAdd) {
        sources.add_inductive(parent_node);
      } else if (!parent_node->is_inductive()) {
        sources.merge(findMemoryAddrSources(
            parent_node, current_func, edge_to_parid));
      }
    }
  }
  if (sources.empty() && current_node->is_compute_op() &&
      !current_node->is_inductive() &&
      current_node->get_static_function() == current_func)
    sources.add_noninductive(current_node);
  sources.sort_and_uniquify();
  return sources;
}
