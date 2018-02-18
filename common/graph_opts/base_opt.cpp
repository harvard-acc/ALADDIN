#include <string>
#include <vector>

#include "base_opt.h"

#include "../DDDG.h"
#include "../ExecNode.h"
#include "../opcode_func.h"
#include "../typedefs.h"

void BaseAladdinOpt::run() {
  static const std::string boundary = "-------------------------------";
  std::cout << boundary << "\n"
            << getCenteredName(boundary.size()) << "\n"
            << boundary << "\n";
  optimize();
}

ExecNode* BaseAladdinOpt::getNodeFromVertex(Vertex vertex) {
  unsigned node_id = vertex_to_name[vertex];
  return exec_nodes.at(node_id);
}

bool BaseAladdinOpt::doesEdgeExist(ExecNode* from, ExecNode* to) {
  if (from != nullptr && to != nullptr)
    return doesEdgeExist(from->get_vertex(), to->get_vertex());
  return false;
}

bool BaseAladdinOpt::doesEdgeExist(Vertex from, Vertex to) {
  return edge(from, to, graph).second;
}

void BaseAladdinOpt::updateGraphWithNewEdges(
    std::vector<NewEdge>& to_add_edges) {
  std::cout << "  Adding " << to_add_edges.size() << " new edges.\n";
  for (auto it = to_add_edges.begin(); it != to_add_edges.end(); ++it) {
    if (*it->from != *it->to && !doesEdgeExist(it->from, it->to)) {
      get(boost::edge_name,
          graph)[add_edge(it->from->get_vertex(), it->to->get_vertex(), graph)
                     .first] = it->parid;
    }
  }
}
void BaseAladdinOpt::updateGraphWithIsolatedNodes(
    std::vector<unsigned>& to_remove_nodes) {
  std::cout << "  Removing " << to_remove_nodes.size() << " isolated nodes.\n";
  for (auto it = to_remove_nodes.begin(); it != to_remove_nodes.end(); ++it) {
    clear_vertex(exec_nodes.at(*it)->get_vertex(), graph);
  }
}

void BaseAladdinOpt::updateGraphWithIsolatedEdges(
    std::set<Edge>& to_remove_edges) {
  std::cout << "  Removing " << to_remove_edges.size() << " edges.\n";
  for (auto it = to_remove_edges.begin(), E = to_remove_edges.end(); it != E;
       ++it)
    remove_edge(source(*it, graph), target(*it, graph), graph);
}

bool BaseAladdinOpt::isPrunableNode(ExecNode* node) const {
  unsigned node_microop = node->get_microop();
  // Certain types of operations modify program state or control flow without
  // necessarily inducing a dependency on future nodes. Such nodes should never
  // be removed from the graph.
  switch (node_microop) {
    case LLVM_IR_SilentStore:
    case LLVM_IR_Store:
    case LLVM_IR_Ret:
      return false;
    default:
      break;
  }

  // DMA stores modify host memory so they cannot be pruned. DMA loads may
  // appear isolated, but in fact it could affect program execution in a future
  // invocation of the same accelerator, so they cannot be pruned either.
  //
  // Branch nodes may have zero children -- this might be the result of loop
  // pipelining -- and they cannot be removed because this could result in the
  // ancestors of the branch nodes also being removed.
  if (node->is_dma_op() || node->is_branch_op())
    return false;

  // In ready mode, we don't add any memory dependencies between dmaLoads and
  // future loads/stores, because this dependence is captured by the full/empty
  // bits. Therefore, a dmaLoad node may have no zero children.
  if (user_params.ready_mode && (node->is_dma_load() || node->is_set_ready_bits()))
    return false;

  // This is probably an error on the user's part; issue a warning.
  if (!user_params.ready_mode && node->is_set_ready_bits()) {
    std::cerr << "Warning: ready mode is not enabled, so calling "
                 "setReadyBits() at node "
              << node->get_node_id() << " has no effect.\n";
    return true;
  }

  return true;
}

void BaseAladdinOpt::cleanLeafNodes() {
  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);

  // Track the number of children each node has that will be removed.
  std::map<unsigned, unsigned> num_children_to_be_removed;
  for (auto& node_it : exec_nodes)
    num_children_to_be_removed[node_it.first] = 0;
  std::vector<unsigned> to_remove_nodes;

  std::vector<Vertex> topo_nodes;
  boost::topological_sort(graph, std::back_inserter(topo_nodes));
  // bottom nodes first
  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi) {
    Vertex node_vertex = *vi;
    if (boost::degree(node_vertex, graph) == 0)
      continue;
    unsigned node_id = vertex_to_name[node_vertex];
    ExecNode* node = exec_nodes.at(node_id);
    if (num_children_to_be_removed.at(node_id) ==
            boost::out_degree(node_vertex, graph) &&
        isPrunableNode(node)) {
      to_remove_nodes.push_back(node_id);
      // This node will be removed, so we need to update the counters for all
      // of its parents.
      in_edge_iter in_edge_it, in_edge_end;
      for (boost::tie(in_edge_it, in_edge_end) = in_edges(node_vertex, graph);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        int parent_id = vertex_to_name[source(*in_edge_it, graph)];
        num_children_to_be_removed.at(parent_id)++;
      }
    } else if (node->is_branch_op()) {
      // Increment the counter for every parent of this branch node with a
      // control dependence. A control dependence is an artifically introduced
      // edge, not a true memory dependence. If that parent's only dependences
      // are control dependences to branches, then that parent's produced value
      // is not used anywhere, so it can be removed. Note that it is the PARENT
      // of the branch that may be removed, not the branch itself.
      in_edge_iter in_edge_it, in_edge_end;
      for (boost::tie(in_edge_it, in_edge_end) = in_edges(node_vertex, graph);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        if (edge_to_parid[*in_edge_it] == CONTROL_EDGE) {
          int parent_id = vertex_to_name[source(*in_edge_it, graph)];
          num_children_to_be_removed.at(parent_id)++;
        }
      }
    }
  }
  updateGraphWithIsolatedNodes(to_remove_nodes);
}

SrcTypes::UniqueLabel BaseAladdinOpt::getUniqueLabel(ExecNode* node) {
  // We'll only find a label if the labelmap is present in the dynamic trace,
  // but if the configuration file doesn't use labels (it's an older config
  // file), we have to fallback on using line numbers.
  auto range = labelmap.equal_range(node->get_line_num());
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second.get_function() != node->get_static_function())
      continue;
    return it->second;
  }
  return SrcTypes::UniqueLabel();
}

unrolling_config_t::const_iterator BaseAladdinOpt::getUnrollFactor(
    ExecNode* node) {
  using namespace SrcTypes;
  UniqueLabel unrolling_id = getUniqueLabel(node);
  auto config_it = user_params.unrolling.find(unrolling_id);
  if (config_it != user_params.unrolling.end())
    return config_it;
  Label* label = src_manager.get<Label>(std::to_string(node->get_line_num()));
  // TODO: Line numbers are no longer actually used as a standalone identifier,
  // so don't specify it. That field can be removed in the future.
  return user_params.unrolling.find(
      UniqueLabel(node->get_static_function(), label));
}
