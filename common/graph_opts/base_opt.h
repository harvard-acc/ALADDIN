#ifndef _BASE_OPT_H_
#define _BASE_OPT_H_

#include <string>
#include <vector>

#include "common/ExecNode.h"
#include "common/opcode_func.h"
#include "common/typedefs.h"
#include "common/user_config.h"
#include "common/graph_analysis_utils.h"

class BaseAladdinOpt {
 public:
  BaseAladdinOpt(ExecNodeMap& _exec_nodes,
                 Graph& _graph,
                 std::vector<DynLoopBound>& _loop_bounds,
                 const VertexNameMap& _vertex_to_name,
                 const labelmap_t& _labelmap,
                 const SrcTypes::SourceManager& _src_manager,
                 const call_arg_map_t& _call_argument_map,
                 const UserConfigParams& _user_params)
      : exec_nodes(_exec_nodes), graph(_graph), loop_bounds(_loop_bounds),
        vertex_to_name(_vertex_to_name), labelmap(_labelmap),
        src_manager(_src_manager), call_argument_map(_call_argument_map),
        user_params(_user_params) {}

  void run();
  virtual void optimize() = 0;
  virtual std::string getCenteredName(size_t size) = 0;

 protected:
  struct NewEdge {
    ExecNode* from;
    ExecNode* to;
    int parid;
  };

  ExecNode* getNodeFromVertex(Vertex vertex);
  bool doesEdgeExist(ExecNode* from, ExecNode* to);
  bool doesEdgeExist(Vertex from, Vertex to);

  unrolling_config_t::const_iterator getUnrollFactor(ExecNode* node);
  SrcTypes::UniqueLabel getUniqueLabel(ExecNode* node);

  void updateGraphWithNewEdges(std::vector<NewEdge>& to_add_edges);
  void updateGraphWithIsolatedNodes(std::vector<unsigned>& to_remove_nodes);
  void updateGraphWithIsolatedEdges(std::set<Edge>& to_remove_edges);
  void cleanLeafNodes();

  // Mutable properties of nodes, loops, and graphs.
  ExecNodeMap& exec_nodes;
  Graph& graph;
  std::vector<DynLoopBound>& loop_bounds;

  // Unmutable graph properties.
  const VertexNameMap& vertex_to_name;

  // Source information.
  const labelmap_t& labelmap;
  const SrcTypes::SourceManager& src_manager;
  const call_arg_map_t& call_argument_map;

  // User configuration settings.
  const UserConfigParams& user_params;
};

#endif
