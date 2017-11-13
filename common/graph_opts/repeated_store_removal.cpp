#include "repeated_store_removal.h"

#include "../DDDG.h"

std::string RepeatedStoreRemoval::getCenteredName(size_t size) {
  return "    Remove Repeated Stores     ";
}

void RepeatedStoreRemoval::optimize() {
  if (!user_params.unrolling.size() && loop_bounds.size() <= 2)
    return;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);

  unsigned removed_stores = 0;
  if (exec_nodes.empty() == 0)
    return;

  int node_id = exec_nodes.size() - 1;
  auto bound_it = loop_bounds.end();
  bound_it--;
  bound_it--;
  while (node_id >= 0) {
    std::unordered_map<unsigned, int> address_store_map;

    while (node_id >= (int)bound_it->node_id && node_id >= 0) {
      ExecNode* node = exec_nodes.at(node_id);
      if (!node->has_vertex() ||
          boost::degree(node->get_vertex(), graph) == 0 ||
          !node->is_store_op()) {
        --node_id;
        continue;
      }
      Addr node_address = node->get_mem_access()->vaddr;
      auto addr_it = address_store_map.find(node_address);

      if (addr_it == address_store_map.end())
        address_store_map[node_address] = node_id;
      else {
        // remove this store, unless it is a dynamic store which cannot be
        // statically disambiguated.
        if (!node->is_dynamic_mem_op()) {
          if (boost::out_degree(node->get_vertex(), graph) == 0) {
            node->set_microop(LLVM_IR_SilentStore);
            removed_stores++;
          } else {
            int num_of_real_children = 0;
            out_edge_iter out_edge_it, out_edge_end;
            for (boost::tie(out_edge_it, out_edge_end) =
                     out_edges(node->get_vertex(), graph);
                 out_edge_it != out_edge_end;
                 ++out_edge_it) {
              if (edge_to_parid[*out_edge_it] != CONTROL_EDGE)
                num_of_real_children++;
            }
            if (num_of_real_children == 0) {
              node->set_microop(LLVM_IR_SilentStore);
              removed_stores++;
            }
          }
        }
      }
      --node_id;
    }

    if (bound_it == loop_bounds.begin())
      break;
    --bound_it;
  }
  cleanLeafNodes();
}
