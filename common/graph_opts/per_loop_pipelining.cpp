#include "per_loop_pipelining.h"

#include "../DDDG.h"

// Per loop pipelining.
//
// This follows a similar implementation strategy as the global pipelining
// pass. The difference is that the global pipelining pass attempts to pipeline
// every loop discovered, whereas this pass only pipelines loops the user
// specifies.  These two passes cannot be used simultaneously.

using namespace SrcTypes;

std::string PerLoopPipelining::getCenteredName(size_t size) {
  return "      Per Loop Pipelining      ";
}

// This does a DFS search of the loop tree to find all the loops to be
// pipelined. For every loop, it checks if any of its children loops needs to be
// pipelined and store those in a list, which is passed to the pipelining
// algorithm. By doing this, we ensure no pipelining occurs across the parent
// loop's boundaries.
void PerLoopPipelining::findAndPipelineLoops(
    LoopIteration* loop,
    std::set<Edge>& to_remove_edges,
    std::vector<NewEdge>& to_add_edges) {
  if (loop->children.size() > 0) {
    std::unordered_map<SrcTypes::UniqueLabel, std::list<LoopIteration*>>
        pipelined_loops;
    for (auto& child : loop->children) {
      findAndPipelineLoops(child, to_remove_edges, to_add_edges);
      if (user_params.pipeline.find(*child->label) !=
          user_params.pipeline.end()) {
        pipelined_loops[*child->label].push_back(child);
        child->pipelined = true;
      }
    }
    for (auto& loop_list: pipelined_loops)
      optimize(loop_list.second, to_remove_edges, to_add_edges);
  }
}

void PerLoopPipelining::optimize() {
  if (loop_bounds.size() <= 2 || user_params.pipeline.size() == 0)
    return;

  if (user_params.global_pipelining) {
    std::cerr
        << "Per loop pipelining cannot be used in conjunction with the "
           "legacy global pipelining! Please disable the global pipelining "
           "setting first.\n";
    return;
  }

  std::set<Edge> to_remove_edges;
  std::vector<NewEdge> to_add_edges;

  findAndPipelineLoops(
      program.loop_info.getRootNode(), to_remove_edges, to_add_edges);

  updateGraphWithNewEdges(to_add_edges);
  updateGraphWithIsolatedEdges(to_remove_edges);
  cleanLeafNodes();
}

void PerLoopPipelining::optimize(std::list<LoopIteration*>& loops,
                                 std::set<Edge>& to_remove_edges,
                                 std::vector<NewEdge>& to_add_edges) {
  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);

  // Strategy: for every loop I want to pipeline:
  //  1. Find the first non-isolated node (FNIN) from this iteration.
  //  2. For all control dependences going to instructions in an iteration and
  //     originating from the branch node that marked the boundary of the
  //     previous iteration:
  //     a. Change the source to the FNIN of the previous iteration.

  // Step 1.
  std::map<unsigned, unsigned> first_non_isolated_nodes;
  for (auto bound_it = loops.begin(), end_it = loops.end(); bound_it != end_it;
       ++bound_it) {
    unsigned curr_bound_node_id = (*bound_it)->start_node->get_node_id();
    unsigned next_bound_node_id = (*bound_it)->end_node->get_node_id();
    auto it = exec_nodes.find(curr_bound_node_id);
    // Find the FNIN after this boundary node (so, don't consider the
    // boundary node itself).
    ++it;
    unsigned node_id = it->first;
    bool is_isolated = true;
    while (node_id < next_bound_node_id) {
      ExecNode* next_node = it->second;
      is_isolated = (!next_node->has_vertex() ||
                     boost::degree(next_node->get_vertex(), graph) == 0 ||
                     next_node->is_branch_op());
      if (!is_isolated) {
        // Use the next bound node as the key, not the current boundary node,
        // because this is exactly how we will change the control dependence
        // edge sources in the next step.
        first_non_isolated_nodes[next_bound_node_id] = node_id;
        // Because now the loop entry becomes the FNIN, we need to update that
        // information in LoopInfo.
        (*bound_it)->start_node = exec_nodes.at(node_id);
        break;
      } else {
        it++;
        node_id = it->first;
      }
    }
  }

  // Step 2.
  ExecNode* prev_branch_node = nullptr;
  ExecNode* prev_first_node = nullptr;
  for (auto first_it = first_non_isolated_nodes.begin(),
            end_it = first_non_isolated_nodes.end();
       first_it != end_it;
       ++first_it) {
    ExecNode* branch_node = exec_nodes.at(first_it->first);
    ExecNode* first_node = exec_nodes.at(first_it->second);
    if (!prev_branch_node && !prev_first_node) {
      prev_branch_node = branch_node;
      prev_first_node = first_node;
      continue;
    }

    if (!doesEdgeExist(prev_first_node, first_node)) {
      to_add_edges.push_back({ prev_first_node, first_node, CONTROL_EDGE });
    }
    // Identify all nodes in the body of the previous iteration that succeed
    // the FNIN of that iteration. If any of these nodes have a control
    // dependence on that iteration's entry branch node, change the source of
    // that edge to be the FNIN.
    //
    // This pass just adds that new edge. The original edge will be removed
    // later.
    assert(prev_branch_node->has_vertex());
    out_edge_iter out_edge_it, out_edge_end;
    for (boost::tie(out_edge_it, out_edge_end) =
             out_edges(prev_branch_node->get_vertex(), graph);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      Vertex child_vertex = target(*out_edge_it, graph);
      ExecNode* child_node = getNodeFromVertex(child_vertex);
      if (*child_node >= *first_node &&
          edge_to_parid[*out_edge_it] == CONTROL_EDGE) {
        // TODO: What is the meaning of EDGE = 1? It gets used in Load/Store
        // Buffer.
        if (!doesEdgeExist(first_node, child_node))
          to_add_edges.push_back({ first_node, child_node, 1 });
      }
    }

    // Pipelining causes the first non-isolated nodes to be the real
    // iteration bounds (rather than branch nodes), so any dependences of
    // FNINs must be converted to control edges.
    assert(first_node->has_vertex());
    in_edge_iter in_edge_it, in_edge_end;
    for (boost::tie(in_edge_it, in_edge_end) =
             in_edges(first_node->get_vertex(), graph);
         in_edge_it != in_edge_end;
         ++in_edge_it) {
      Vertex parent_vertex = source(*in_edge_it, graph);
      ExecNode* parent_node = getNodeFromVertex(parent_vertex);
      // TODO: Ignore branch nodes since they are typically loop bounds, but
      // what if we have control flow with a loop?
      if (!parent_node->is_branch_op()) {
        to_remove_edges.insert(*in_edge_it);
        to_add_edges.push_back({ parent_node, first_node, CONTROL_EDGE });
      }
    }

    // Remove all control dependences from the previous branch node to its
    // children (because these dependences have been moved to the previous
    // FNIN), except for call nodes (we don't optimize across function
    // boundaries).
    assert(prev_branch_node->has_vertex());
    for (boost::tie(out_edge_it, out_edge_end) =
             out_edges(prev_branch_node->get_vertex(), graph);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      unsigned target_node_id = vertex_to_name[target(*out_edge_it, graph)];
      if (exec_nodes.at(target_node_id)->is_call_op())
        continue;
      if (edge_to_parid[*out_edge_it] != CONTROL_EDGE)
        continue;
      to_remove_edges.insert(*out_edge_it);
    }
    prev_branch_node = branch_node;
    prev_first_node = first_node;
  }
}
