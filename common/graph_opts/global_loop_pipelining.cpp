#include "global_loop_pipelining.h"
#include "../DDDG.h"

// Global loop pipelining.
//
// After loop unrolling, we define strict control dependences between basic
// block, where all the instructions in the following basic block depend on the
// prev branch instruction. To support loop pipelining, which allows the next
// iteration starting without waiting until the prev iteration finish, we move
// the control dependences between last branch node in the prev basic block and
// instructions in the next basic block to the first non isolated instruction
// in the prev basic block and instructions in the next basic block...

std::string GlobalLoopPipelining::getCenteredName(size_t size) {
  return "         Loop Pipelining        ";
}

void GlobalLoopPipelining::optimize() {
  if (!user_params.global_pipelining) {
    std::cerr << "Global loop pipelining is not ON." << std::endl;
    return;
  }

  if (!user_params.unrolling.size()) {
    std::cerr << "Loop Unrolling is not defined. " << std::endl;
    std::cerr << "Loop pipelining is only applied to unrolled loops."
              << std::endl;
    return;
  }

  if (loop_bounds.size() <= 2)
    return;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);

  vertex_iter vi, vi_end;
  std::set<Edge> to_remove_edges;
  std::vector<NewEdge> to_add_edges;

  // first_non_isolated_node stores mappings between a loop boundary and its
  // first non isolated node.
  std::map<unsigned, unsigned> first_non_isolated_node;
  auto bound_it = loop_bounds.begin();
  auto node_it = exec_nodes.begin();
  ExecNode* curr_node = exec_nodes.at(bound_it->node_id);
  bound_it++;  // skip first region
  while (node_it != exec_nodes.end()) {
    assert(exec_nodes.at(bound_it->node_id)->is_branch_op());
    while (node_it != exec_nodes.end() && node_it->first < bound_it->node_id) {
      curr_node = node_it->second;
      if (!curr_node->has_vertex() ||
          boost::degree(curr_node->get_vertex(), graph) == 0 ||
          curr_node->is_branch_op()) {
        ++node_it;
        continue;
      } else {
        first_non_isolated_node[bound_it->node_id] = curr_node->get_node_id();
        node_it = exec_nodes.find(bound_it->node_id);
        assert(node_it != exec_nodes.end());
        break;
      }
    }
    if (first_non_isolated_node.find(bound_it->node_id) ==
        first_non_isolated_node.end())
      first_non_isolated_node[bound_it->node_id] = bound_it->node_id;
    bound_it++;
    if (bound_it == loop_bounds.end() - 1)
      break;
  }

  ExecNode* prev_branch_n = nullptr;
  ExecNode* prev_first_n = nullptr;
  for (auto first_it = first_non_isolated_node.begin(),
            E = first_non_isolated_node.end();
       first_it != E;
       ++first_it) {
    ExecNode* br_node = exec_nodes.at(first_it->first);
    ExecNode* first_node = exec_nodes.at(first_it->second);
    bool found = false;
    auto unroll_it = getUnrollFactor(br_node);
    /* We only want to pipeline loop iterations that are from the same unrolled
     * loop. Here we first check the current basic block is part of an unrolled
     * loop iteration. */
    if (unroll_it != user_params.unrolling.end()) {
      // check whether the previous branch is the same loop or not
      if (prev_branch_n != nullptr) {
        if ((br_node->get_line_num() == prev_branch_n->get_line_num()) &&
            (br_node->get_static_function() ==
             prev_branch_n->get_static_function()) &&
            first_node->get_line_num() == prev_first_n->get_line_num()) {
          found = true;
        }
      }
    }
    /* We only pipeline matching loop iterations. */
    if (!found) {
      prev_branch_n = br_node;
      prev_first_n = first_node;
      continue;
    }
    // adding dependence between prev_first and first_id
    if (!doesEdgeExist(prev_first_n, first_node)) {
      to_add_edges.push_back({ prev_first_n, first_node, CONTROL_EDGE });
    }
    // adding dependence between first_id and prev_branch's children
    assert(prev_branch_n->has_vertex());
    out_edge_iter out_edge_it, out_edge_end;
    for (boost::tie(out_edge_it, out_edge_end) =
             out_edges(prev_branch_n->get_vertex(), graph);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      Vertex child_vertex = target(*out_edge_it, graph);
      ExecNode* child_node = getNodeFromVertex(child_vertex);
      if (*child_node < *first_node ||
          edge_to_parid[*out_edge_it] != CONTROL_EDGE)
        continue;
      if (!doesEdgeExist(first_node, child_node)) {
        to_add_edges.push_back({ first_node, child_node, 1 });
      }
    }
    // update first_id's parents, dependence become strict control dependence
    assert(first_node->has_vertex());
    in_edge_iter in_edge_it, in_edge_end;
    for (boost::tie(in_edge_it, in_edge_end) =
             in_edges(first_node->get_vertex(), graph);
         in_edge_it != in_edge_end;
         ++in_edge_it) {
      Vertex parent_vertex = source(*in_edge_it, graph);
      ExecNode* parent_node = getNodeFromVertex(parent_vertex);
      // unsigned parent_id = vertex_to_name[parent_vertex];
      if (parent_node->is_branch_op())
        continue;
      to_remove_edges.insert(*in_edge_it);
      to_add_edges.push_back({ parent_node, first_node, CONTROL_EDGE });
    }
    // remove control dependence between prev br node to its children
    assert(prev_branch_n->has_vertex());
    for (boost::tie(out_edge_it, out_edge_end) =
             out_edges(prev_branch_n->get_vertex(), graph);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      if (exec_nodes.at(vertex_to_name[target(*out_edge_it, graph)])->is_call_op())
        continue;
      if (edge_to_parid[*out_edge_it] != CONTROL_EDGE)
        continue;
      to_remove_edges.insert(*out_edge_it);
    }
    prev_branch_n = br_node;
    prev_first_n = first_node;
  }

  updateGraphWithNewEdges(to_add_edges);
  updateGraphWithIsolatedEdges(to_remove_edges);
  cleanLeafNodes();
}
