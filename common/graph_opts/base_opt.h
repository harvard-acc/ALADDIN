#ifndef _BASE_OPT_H_
#define _BASE_OPT_H_

#include <string>
#include <vector>

#include "../ExecNode.h"
#include "../Program.h"
#include "../opcode_func.h"
#include "../typedefs.h"
#include "../user_config.h"

class BaseAladdinOpt {
 public:
  // Construct an optimization object.
  //
  // Ensure that the constructor argument @program is NOT const qualified and
  // that all elements in the initializer list use this non-const Program
  // reference. This lets us use a mixture of const and non-const Program
  // fields while keeping a const reference to Program for its helper
  // functions.
  BaseAladdinOpt(Program& _program,
                 const SrcTypes::SourceManager& _src_manager,
                 const UserConfigParams& _user_params)
      : program(_program), exec_nodes(_program.nodes), graph(_program.graph),
        loop_bounds(_program.loop_bounds),
        vertex_to_name(_program.vertex_to_name), labelmap(_program.labelmap),
        src_manager(_src_manager), call_argument_map(_program.call_arg_map),
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

  // XXX: These are accessible from program.
  ExecNode* getNodeFromVertex(Vertex vertex);
  bool doesEdgeExist(ExecNode* from, ExecNode* to);
  bool doesEdgeExist(Vertex from, Vertex to);

  // Is this node removable from the program if it has no children?
  bool isPrunableNode(ExecNode* node) const;

  unrolling_config_t::const_iterator getUnrollFactor(ExecNode* node);
  SrcTypes::UniqueLabel getUniqueLabel(ExecNode* node);

  void updateGraphWithNewEdges(std::vector<NewEdge>& to_add_edges);
  void updateGraphWithIsolatedNodes(std::vector<unsigned>& to_remove_nodes);
  void updateGraphWithIsolatedEdges(std::set<Edge>& to_remove_edges);
  void cleanLeafNodes();

  const Program& program;

  // Mutable properties of nodes, loops, and graphs.
  ExecNodeMap& exec_nodes;
  Graph& graph;
  std::vector<DynLoopBound>& loop_bounds;

  // Unmutable graph properties.
  const VertexNameMap& vertex_to_name;

  // Source information.
  const labelmap_t& labelmap;
  const SrcTypes::SourceManager& src_manager;
  const CallArgMap& call_argument_map;

  // User configuration settings.
  const UserConfigParams& user_params;
};

#endif
