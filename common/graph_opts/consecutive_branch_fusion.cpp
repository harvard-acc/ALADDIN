#include "consecutive_branch_fusion.h"

#include "../DDDG.h"

// Consecutive branch fusion.
//
// Aladdin enforces control dependences for all branch nodes. But sometimes we
// will see multiple branch nodes consecutively, particularly in the case of
// multiply nested loops and preheader basic blocks. This can introduce a
// significant amount of additional latency. This optimization assumes that an
// FSM can always determine in a single cycle which loop nest to execute next
// and fuses consecutive branch nodes together.

std::string ConsecutiveBranchFusion::getCenteredName(size_t size) {
  return "   Fuse consecutive branches   ";
}

void ConsecutiveBranchFusion::optimize() {
  std::set<Edge> to_remove_edges;
  std::vector<NewEdge> to_add_edges;

  std::list<Vertex> topo_nodes;
  boost::topological_sort(graph, std::front_inserter(topo_nodes));

  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi) {
    Vertex vertex = *vi;
    ExecNode* node = getNodeFromVertex(vertex);
    if (!(node->is_branch_op() || node->is_call_op()))
      continue;
    if (boost::out_degree(vertex, graph) != 1)
      continue;

    std::list<ExecNode*> branch_chain{ node };
    findBranchChain(node, branch_chain, to_remove_edges);
    if (branch_chain.size() > 1) {
      for (auto it = branch_chain.begin(); it != --branch_chain.end();)
        to_add_edges.push_back({ *it, *(++it), FUSED_BRANCH_EDGE });
    }
  }

  updateGraphWithIsolatedEdges(to_remove_edges);
  updateGraphWithNewEdges(to_add_edges);
  cleanLeafNodes();
}

void ConsecutiveBranchFusion::findBranchChain(
    ExecNode* root,
    std::list<ExecNode*>& branch_chain,
    std::set<Edge>& to_remove_edges) {

  if (boost::out_degree(root->get_vertex(), graph) != 1)
    return;
  out_edge_iter out_edge_it, out_edge_end;
  for (boost::tie(out_edge_it, out_edge_end) =
           out_edges(root->get_vertex(), graph);
       out_edge_it != out_edge_end;
       ++out_edge_it) {
    Vertex target_vertex = target(*out_edge_it, graph);
    ExecNode* target_node = getNodeFromVertex(target_vertex);
    if (target_node->is_branch_op() || target_node->is_call_op()) {
      branch_chain.push_back(target_node);
      to_remove_edges.insert(*out_edge_it);
      findBranchChain(target_node, branch_chain, to_remove_edges);
    }
  }
}

