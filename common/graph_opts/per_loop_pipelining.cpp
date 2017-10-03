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

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);
  std::set<Edge> to_remove_edges;
  std::vector<NewEdge> to_add_edges;

  // Strategy: for every loop I want to pipeline:
  //  1. Find every node that starts an iteration of this loop.
  //  2. Find the first non-isolated node (FNIN) from this iteration.
  //  3. For all control dependences going to instructions in an iteration and
  //     originating from the branch node that marked the boundary of the
  //     previous iteration:
  //     a. Change the source to the FNIN of the previous iteration.
  for (const UniqueLabel& loop : user_params.pipeline) {
    // Step 1.
    std::list<cnode_pair_t> current_loop_bounds =
        program.findLoopBoundaries(loop);

    // Step 2.
    std::map<unsigned, unsigned> first_non_isolated_nodes;
    for (auto bound_it = current_loop_bounds.begin(),
              end_it = current_loop_bounds.end();
         bound_it != end_it; ++bound_it) {
      unsigned curr_bound_node_id = bound_it->first->get_node_id();
      unsigned next_bound_node_id = bound_it->second->get_node_id();
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
          break;
        } else {
          it++;
          node_id = it->first;
        }
      }
    }

    // Step 3.
    ExecNode* prev_branch_node = nullptr;
    ExecNode* prev_first_node = nullptr;
    for (auto first_it = first_non_isolated_nodes.begin(),
              end_it = first_non_isolated_nodes.end();
         first_it != end_it;
         ++first_it) {
      ExecNode* branch_node = exec_nodes.at(first_it->first);
      ExecNode* first_node = exec_nodes.at(first_it->second);
      if (!prev_branch_node && !prev_first_node) {
        prev_branch_node = branch_node; prev_first_node = first_node;
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

  updateGraphWithNewEdges(to_add_edges);
  updateGraphWithIsolatedEdges(to_remove_edges);
  cleanLeafNodes();
}
