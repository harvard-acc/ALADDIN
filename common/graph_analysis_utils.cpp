#include <list>

#include "ExecNode.h"
#include "SourceEntity.h"
#include "typedefs.h"
#include "graph_analysis_utils.h"

using namespace SrcTypes;

ExecNode* getNextNode(ExecNodeMap& exec_nodes, unsigned node_id) {
  auto it = exec_nodes.find(node_id);
  assert(it != exec_nodes.end());
  auto next_it = std::next(it);
  if (next_it == exec_nodes.end())
    return nullptr;
  return next_it->second;
}

std::list<node_pair_t> findLoopBoundaries(
    Graph& graph,
    std::vector<DynLoopBound>& loop_bounds,
    ExecNodeMap& exec_nodes,
    const UniqueLabel& loop_label) {
  std::list<node_pair_t> loop_boundaries;
  bool is_loop_executing = false;
  int current_loop_depth = -1;

  ExecNode* loop_start;

  // The loop boundaries provided by the accelerator are in a linear list with
  // no structure. We need to identify the start and end of each unrolled loop
  // section and add the corresponding pair of nodes to loop_boundaries.
  //
  // The last loop bound node is one past the last node, so we'll ignore it.
  for (auto it = loop_bounds.begin(); it != --loop_bounds.end(); ++it) {
    const DynLoopBound& loop_bound = *it;
    ExecNode* node = exec_nodes[loop_bound.node_id];
    Vertex vertex = node->get_vertex();
    if (boost::degree(vertex, graph) > 0 &&
        node->get_line_num() == loop_label.get_line_number() &&
        node->get_static_function() == loop_label.get_function()) {
      if (!is_loop_executing) {
        if (loop_bound.target_loop_depth > node->get_loop_depth()) {
          is_loop_executing = true;
          loop_start = node;
          current_loop_depth = loop_bound.target_loop_depth;
        }
      } else {
        if (loop_bound.target_loop_depth == current_loop_depth) {
          // We're repeating an iteration of the same loop body.
          loop_boundaries.push_back(std::make_pair(loop_start, node));
          loop_start = node;
        } else if (loop_bound.target_loop_depth < current_loop_depth) {
          // We've left the loop.
          loop_boundaries.push_back(std::make_pair(loop_start, node));
          is_loop_executing = false;
          loop_start = NULL;
        } else if (loop_bound.target_loop_depth > current_loop_depth) {
          // We've entered an inner loop nest. We're not interested in the
          // inner loop nest.
          continue;
        }
      }
    }
  }
  return loop_boundaries;
}
