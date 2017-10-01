#ifndef _GRAPH_ANALYSIS_UTILS_H_
#define _GRAPH_ANALYSIS_UTILS_H_

#include "SourceEntity.h"

/* A dynamic loop boundary is identified by a branch/call node and a target
 * loop depth.
 */
struct DynLoopBound {
  unsigned node_id;
  unsigned target_loop_depth;
  DynLoopBound(unsigned _node_id, unsigned _target_loop_depth)
      : node_id(_node_id), target_loop_depth(_target_loop_depth) {}
};

ExecNode* getNextNode(ExecNodeMap& exec_nodes, unsigned node_id);

// TODO: Make loop_bounds hold the ExecNode* pointer directly, so we can
// remove the exec_nodes parameter.
std::list<node_pair_t> findLoopBoundaries(
    Graph& graph,
    std::vector<DynLoopBound>& loop_bounds,
    ExecNodeMap& exec_nodes,
    const SrcTypes::UniqueLabel& loop_label);

#endif
