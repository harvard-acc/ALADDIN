#include <stack>

#include "loop_unrolling.h"

#include "../DDDG.h"

using namespace SrcTypes;

std::ostream& operator<<(std::ostream& os, const LoopBoundDescriptor& obj) {
  os << "LoopBoundDescriptor(microop = " << obj.microop
     << ", loop_depth = " << obj.loop_depth
     << ", call depth = " << obj.call_depth
     << ", dyn_invocations = " << obj.dyn_invocations
     << ")";

  return os;
}

std::string LoopUnrolling::getCenteredName(size_t size) {
  return "         Loop Unrolling        ";
}

void LoopUnrolling::optimize() {
  if (!user_params.unrolling.size()) {
    std::cerr << "No loop unrolling configuration options found." << std::endl;
    return;
  }
  std::stack<LoopBoundDescriptor> loop_nests;
  std::vector<unsigned> to_remove_nodes;
  std::vector<ExecNode*> nodes_between;
  std::vector<NewEdge> to_add_edges;

  bool first = false;
  int iter_counts = 0;
  unsigned curr_call_depth = 0;
  ExecNode* prev_branch = nullptr;

  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (!node->has_vertex())
      continue;
    if (node->is_ret_op()) {
      curr_call_depth--;
      // Clear all loop nests from the stack in case we return from an inner
      // loop.
      while (!loop_nests.empty() &&
             loop_nests.top().call_depth > curr_call_depth) {
        loop_nests.pop();
      }
    }
    Vertex node_vertex = node->get_vertex();
    // We let all the branch nodes proceed to the unrolling handling no matter
    // whether they are isolated or not. Although most of the branch nodes are
    // connected anyway, one exception is unconditional branch that is not
    // dependent on any nodes. The is_branch_op() check can let the
    // unconditional branch proceed.
    if (boost::degree(node_vertex, graph) == 0 && !node->is_branch_op())
      continue;
    if (user_params.ready_mode && node->is_dma_load())
      continue;
    unsigned node_id = node->get_node_id();
    ExecNode* next_node = program.getNextNode(node_id);
    if (!next_node)
      continue;
    DynLoopBound dyn_bound(node_id, next_node->get_loop_depth());
    if (!first) {
      // prev_branch should not be anything but a branch node.
      if (node->is_branch_op()) {
        first = true;
        loop_bounds.push_back(dyn_bound);
        prev_branch = node;
      } else {
        continue;
      }
    }
    assert(prev_branch != nullptr);
    // We should never add control edges to DMA nodes. They should be
    // constrained by memory dependences only.
    if (prev_branch != node && !node->is_dma_op()) {
      to_add_edges.push_back({ prev_branch, node, CONTROL_EDGE });
    }
    // If the current node is not a branch node, it will not be a boundary node.
    if (!node->is_branch_op()) {
      if (!node->is_dma_op())
        nodes_between.push_back(node);
    } else {
      // for the case that the first non-isolated node is also a call node;
      if (node->is_call_op() && loop_bounds.rbegin()->node_id != node_id) {
        loop_bounds.push_back(dyn_bound);
        curr_call_depth++;
        prev_branch = node;
      }
      auto unroll_it = getUnrollFactor(node);
      if (unroll_it == user_params.unrolling.end() || unroll_it->second == 0) {
        // not unrolling branch
        if (!node->is_call_op() && !node->is_dma_op()) {
          nodes_between.push_back(node);
          continue;
        }
        if (!doesEdgeExist(prev_branch, node) && !node->is_dma_op()) {
          // Enforce dependences between branch nodes, including call nodes
          to_add_edges.push_back({ prev_branch, node, CONTROL_EDGE });
        }
        for (auto prev_node_it = nodes_between.begin(), E = nodes_between.end();
             prev_node_it != E;
             prev_node_it++) {
          if (!doesEdgeExist(*prev_node_it, node)) {
            to_add_edges.push_back({ *prev_node_it, node, CONTROL_EDGE });
          }
        }
        nodes_between.clear();
        nodes_between.push_back(node);
        prev_branch = node;
      } else {  // unrolling the branch
        // This is the loop descriptor we're trying to find in the loop nest
        // stack.
        LoopBoundDescriptor loop_id(
            next_node->get_basic_block(), node->get_microop(),
            next_node->get_loop_depth(), curr_call_depth);
        // Once we've found the loop descriptor, use this pointer to the top of
        // the stack to update the invocations count.
        LoopBoundDescriptor* curr_loop = nullptr;
        // This is used when an exit branch should be identified as a loop bound
        // if the loop size is not multiples of the unrolling factor.
        bool is_exit_loop_bound = false;

        /* Four possibilities with this loop:
         * 1. We're entering a new loop nest (possibly due to a Call).
         * 2. We're exiting the current innermost loop.
         * 3. We're repeating an inner loop.
         * 4. We're jumping to a different loop entirely due to a goto.
         */
        if (loop_nests.empty() ||
            loop_nests.top().loop_depth < loop_id.loop_depth ||
            loop_nests.top().call_depth < curr_call_depth) {
          // If our loop depth is greater than the top of the stack, add a new
          // loop nest.
          loop_nests.push(loop_id);
          curr_loop = &loop_nests.top();
        } else if (loop_nests.top().exitBrIs(loop_id)) {
          curr_loop = &loop_nests.top();
          // We are leaving the loop and by now we know how many iterations this
          // loop has. If the loop's iteration number is not multiples of the
          // unrolling factor, the exit branch should be a loop bound, otherwise
          // we will have an isolated loop bound that messes up the loop tree
          // building process later.
          int unroll_factor = unroll_it->second;
          if ((curr_loop->dyn_invocations + 1) % unroll_factor != 0)
            is_exit_loop_bound = true;
          loop_nests.pop();
        } else if (loop_nests.top() == loop_id) {
          // We're repeating the same loop. Nothing to do.
          curr_loop = &loop_nests.top();
        } else {
          continue;
        }

        // Counting number of loop iterations.
        // The following unrolls the loop if its dynamic invocation number is
        // not multiples of the unrolling factor. Here we don't know yet how
        // many iterations this loop has, and as we have started applying
        // unrolling and detecting loop boundaries, if the loop turns out to
        // have non-multiple iterations of the unrolling factor, we will need to
        // make the exit branch a loop bound as well.
        int unroll_factor = unroll_it->second;
        curr_loop->dyn_invocations++;
        if (curr_loop->dyn_invocations % unroll_factor == 0 ||
            is_exit_loop_bound) {
          if (loop_bounds.rbegin()->node_id != node_id) {
            loop_bounds.push_back(dyn_bound);
          }
          iter_counts++;
          for (auto prev_node_it = nodes_between.begin(),
                    E = nodes_between.end();
               prev_node_it != E;
               prev_node_it++) {
            if (!doesEdgeExist(*prev_node_it, node)) {
              to_add_edges.push_back({ *prev_node_it, node, CONTROL_EDGE });
            }
          }
          nodes_between.clear();
          nodes_between.push_back(node);
          prev_branch = node;
        } else {
          to_remove_nodes.push_back(node_id);
        }
      }
    }
  }
  loop_bounds.push_back(DynLoopBound(exec_nodes.size(), 0));

  if (iter_counts == 0 && user_params.unrolling.size() != 0) {
    std::cerr << "-------------------------------\n"
              << "Loop Unrolling was NOT applied.\n"
              << "Either loop labels or line numbers are incorrect, or the\n"
              << "loop unrolling factor is larger than the loop trip count.\n"
              << "-------------------------------" << std::endl;
  }
  updateGraphWithNewEdges(to_add_edges);
  updateGraphWithIsolatedNodes(to_remove_nodes);
  cleanLeafNodes();
}

std::string LoopFlattening::getCenteredName(size_t size) {
  return "         Loop Flatten          ";
}

void LoopFlattening::optimize() {
  if (!user_params.unrolling.size())
    return;

  std::vector<unsigned> to_remove_nodes;
  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    auto config_it = getUnrollFactor(node);
    if (config_it == user_params.unrolling.end() || config_it->second != 0)
      continue;
    if (node->is_compute_op() && !node->is_inductive())
      node->set_microop(LLVM_IR_Move);
    else if (node->is_branch_op())
      to_remove_nodes.push_back(node->get_node_id());
  }
  updateGraphWithIsolatedNodes(to_remove_nodes);
  cleanLeafNodes();
}
