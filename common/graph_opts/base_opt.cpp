#include <string>
#include <vector>

#include "base_opt.h"

#include "common/DDDG.h"
#include "common/ExecNode.h"
#include "common/opcode_func.h"
#include "common/typedefs.h"

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

void BaseAladdinOpt::cleanLeafNodes() {
  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);

  // Track the number of children each node has.
  std::map<unsigned, int> num_of_children;
  for (auto& node_it : exec_nodes)
    num_of_children[node_it.first] = 0;
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
    int node_microop = node->get_microop();
    if (num_of_children.at(node_id) == boost::out_degree(node_vertex, graph) &&
        node_microop != LLVM_IR_SilentStore && node_microop != LLVM_IR_Store &&
        node_microop != LLVM_IR_Ret && !node->is_branch_op() &&
        !node->is_dma_store()) {
      to_remove_nodes.push_back(node_id);
      // iterate its parents
      in_edge_iter in_edge_it, in_edge_end;
      for (boost::tie(in_edge_it, in_edge_end) = in_edges(node_vertex, graph);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        int parent_id = vertex_to_name[source(*in_edge_it, graph)];
        num_of_children.at(parent_id)++;
      }
    } else if (node->is_branch_op()) {
      // iterate its parents
      in_edge_iter in_edge_it, in_edge_end;
      for (boost::tie(in_edge_it, in_edge_end) = in_edges(node_vertex, graph);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        if (edge_to_parid[*in_edge_it] == CONTROL_EDGE) {
          int parent_id = vertex_to_name[source(*in_edge_it, graph)];
          num_of_children.at(parent_id)++;
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
