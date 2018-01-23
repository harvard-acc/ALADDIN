#include "phi_node_removal.h"

// Phi node removal.
//
// Phi nodes are useful in static compiler analysis but do not need to be
// accounted for during simulation. Remove all phi nodes and attach the phi
// node's parent to all of its children.

std::string PhiNodeRemoval::getCenteredName(size_t size) {
  return "  Remove PHI and Convert Nodes ";
}

void PhiNodeRemoval::optimize() {
  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);
  std::set<Edge> to_remove_edges;
  std::vector<NewEdge> to_add_edges;
  std::unordered_set<ExecNode*> checked_phi_nodes;
  for (auto node_it = exec_nodes.rbegin(); node_it != exec_nodes.rend();
       node_it++) {
    ExecNode* node = node_it->second;
    if (!node->has_vertex() ||
        (!node->is_phi_op() && !node->is_convert_op()) ||
        (checked_phi_nodes.find(node) != checked_phi_nodes.end()))
      continue;
    Vertex node_vertex = node->get_vertex();
    // find its children
    std::vector<std::pair<ExecNode*, int>> phi_child;
    out_edge_iter out_edge_it, out_edge_end;
    for (boost::tie(out_edge_it, out_edge_end) = out_edges(node_vertex, graph);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      checked_phi_nodes.insert(node);
      to_remove_edges.insert(*out_edge_it);
      Vertex v = target(*out_edge_it, graph);
      phi_child.push_back(
          std::make_pair(getNodeFromVertex(v), edge_to_parid[*out_edge_it]));
    }
    if (phi_child.size() == 0 || boost::in_degree(node_vertex, graph) == 0)
      continue;
    if (node->is_phi_op()) {
      // find its first non-phi ancestor.
      // phi node can have multiple children, but it can only have one parent.
      assert(boost::in_degree(node_vertex, graph) == 1);
      in_edge_iter in_edge_it = in_edges(node_vertex, graph).first;
      Vertex parent_vertex = source(*in_edge_it, graph);
      ExecNode* nonphi_ancestor = getNodeFromVertex(parent_vertex);
      // Search for the first non-phi ancestor of the current phi node.
      while (nonphi_ancestor->is_phi_op()) {
        assert(nonphi_ancestor->has_vertex());
        Vertex parent_vertex = nonphi_ancestor->get_vertex();
        if (boost::in_degree(parent_vertex, graph) == 0)
          break;
        to_remove_edges.insert(*in_edge_it);
        in_edge_it = in_edges(parent_vertex, graph).first;
        parent_vertex = source(*in_edge_it, graph);
        nonphi_ancestor = getNodeFromVertex(parent_vertex);
      }
      to_remove_edges.insert(*in_edge_it);
      if (!nonphi_ancestor->is_phi_op()) {
        // Add dependence between the current phi node's children and its first
        // non-phi ancestor.
        for (auto child_it = phi_child.begin(), chil_E = phi_child.end();
             child_it != chil_E;
             ++child_it) {
          to_add_edges.push_back(
              { nonphi_ancestor, child_it->first, child_it->second });
        }
      }
    } else {
      // convert nodes
      assert(boost::in_degree(node_vertex, graph) == 1);
      in_edge_iter in_edge_it = in_edges(node_vertex, graph).first;
      Vertex parent_vertex = source(*in_edge_it, graph);
      ExecNode* nonphi_ancestor = getNodeFromVertex(parent_vertex);
      while (nonphi_ancestor->is_convert_op() || nonphi_ancestor->is_phi_op()) {
        Vertex parent_vertex = nonphi_ancestor->get_vertex();
        if (boost::in_degree(parent_vertex, graph) == 0)
          break;
        to_remove_edges.insert(*in_edge_it);
        in_edge_it = in_edges(parent_vertex, graph).first;
        parent_vertex = source(*in_edge_it, graph);
        nonphi_ancestor = getNodeFromVertex(parent_vertex);
      }
      to_remove_edges.insert(*in_edge_it);
      if (!nonphi_ancestor->is_convert_op() && !nonphi_ancestor->is_phi_op()) {
        for (auto child_it = phi_child.begin(), chil_E = phi_child.end();
             child_it != chil_E;
             ++child_it) {
          to_add_edges.push_back(
              { nonphi_ancestor, child_it->first, child_it->second });
        }
      }
    }
    std::vector<std::pair<ExecNode*, int>>().swap(phi_child);
  }

  updateGraphWithIsolatedEdges(to_remove_edges);
  updateGraphWithNewEdges(to_add_edges);
  cleanLeafNodes();
}
