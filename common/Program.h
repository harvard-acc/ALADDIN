#ifndef _PROGRAM_H_
#define _PROGRAM_H_

#include <list>

#include "DynamicEntity.h"
#include "ExecNode.h"
#include "SourceEntity.h"
#include "LoopInfo.h"
#include "typedefs.h"

/* A dynamic loop boundary is identified by a branch/call node and a target
 * loop depth.
 */
struct DynLoopBound {
  unsigned node_id;
  unsigned target_loop_depth;
  DynLoopBound(unsigned _node_id, unsigned _target_loop_depth)
      : node_id(_node_id), target_loop_depth(_target_loop_depth) {}
};


// This class maintains mappings between names of function call arguments in
// the caller and callee.
//
// This class enables us to use a single canonical name for each array that
// appears in the program, where the canonical name is the name of the array as
// it appears in the top level function. We can then map any argument name in a
// dynamic function invocation to this canonical name.
//
// NOTE: this could just as well achieved by using pointers to identify arrays,
// but that will require a major change to Aladdin.
class CallArgMap {
 protected:
  typedef SrcTypes::DynamicVariable VarId;
  typedef std::unordered_map<VarId, VarId> _map;

 public:
  CallArgMap() {}

  // Return the canonical name of the given argument.
  VarId lookup(VarId& reg_id) const {
    auto it = call_arg_map.find(reg_id);
    while (it != call_arg_map.end()) {
      reg_id = it->second;
      it = call_arg_map.find(reg_id);
    }
    return reg_id;
  }

  // Add a caller to callee mapping.
  void add(VarId& callee_reg_id, VarId& caller_reg_id) {
    call_arg_map[callee_reg_id] = caller_reg_id;
  }

  void clear() { call_arg_map.clear(); }

 protected:
  _map call_arg_map;
};

// An Aladdin program object.
//
// The program is defined primarily by the data dependence graph and the nodes
// of the graph.
class Program {
 public:
  Program() : loop_info(this) {}

  void clear();
  void clearExecNodes();

  // Graph modifiers.
  void addEdge(unsigned int from, unsigned int to, uint8_t parid);
  ExecNode* insertNode(unsigned node_id, uint8_t microop);

  void createVertexMap() { vertex_to_name = get(boost::vertex_node_id, graph); }

  // Return the node with the next higher node id, or nullptr if there is none.
  ExecNode* getNextNode(unsigned node_id) const;

  //=-------- Graph accessors ---------=//

  // Return all nodes with dependencies originating from OR leaving this node.
  std::vector<unsigned> getConnectedNodes(unsigned int node_id) const;
  std::vector<unsigned> getParentNodes(unsigned int node_id) const;
  std::vector<unsigned> getChildNodes(unsigned int node_id) const;

  int getNumNodes() const { return boost::num_vertices(graph); }
  int getNumEdges() const { return boost::num_edges(graph); }
  int getNumConnectedNodes(unsigned int node_id) const {
    return boost::degree(nodes.at(node_id)->get_vertex(), graph);
  }

  bool edgeExistsV(Vertex from, Vertex to) const {
    return edge(from, to, graph).second;
  }

  bool edgeExists(const ExecNode* from, const ExecNode* to) const {
    if (from != nullptr && to != nullptr)
      return edgeExistsV(from->get_vertex(), to->get_vertex());
    return false;
  }

  bool edgeExists(unsigned int from, unsigned int to) const {
    return edgeExists(nodes.at(from), nodes.at(to));
  }

  bool edgeExists(unsigned int node_id) const {
    return nodes.find(node_id) != nodes.end();
  }

  unsigned atVertex(const Vertex& vertex) const {
    return vertex_to_name[vertex];
  }

  ExecNode* nodeAtVertex(const Vertex& vertex) const {
    return nodes.at(vertex_to_name[vertex]);
  }

  bool nodeExists(unsigned node_id) const {
    return nodes.find(node_id) != nodes.end();
  }

  //=------------ Commonly used accessors ---------------=//

  std::string getBaseAddressLabel(unsigned int node_id) const {
    return nodes.at(node_id)->get_array_label();
  }

  unsigned getPartitionIndex(unsigned int node_id) const {
    return nodes.at(node_id)->get_partition_index();
  }

  // Return the UniqueLabel associated with this node.
  //
  // If the labelmap does not contain an entry corresponding to this node's
  // line number, then an empty UniqueLabel is returned.
  SrcTypes::UniqueLabel getUniqueLabel(ExecNode* node) const;

  // Get the edge weight between these two nodes, or -1 if no edge exists.
  int getEdgeWeight(unsigned node_id_0, unsigned node_id_1) const;

  //=------------ Graph analysis functions ---------------=//

  // Helper function for the following two findLoopBoundaries, either finding a
  // static unique label or a dynamic label.
  std::list<cnode_pair_t> findLoopBoundaries(
      const SrcTypes::UniqueLabel* static_label,
      const SrcTypes::DynamicLabel* dynamic_label) const;

  // Return pairs of branch nodes bounding iterations of the specified loop.
  std::list<cnode_pair_t> findLoopBoundaries(
      const SrcTypes::UniqueLabel& loop_label) const;

  // Return pairs of branch nodes bounding iterations of the specified dynamic
  // loop.
  std::list<cnode_pair_t> findLoopBoundaries(
      const SrcTypes::DynamicLabel& loop_label) const;

  // Return the Call and Return node pairs which bound a function.
  std::list<cnode_pair_t> findFunctionBoundaries(SrcTypes::Function* func) const;

  // Find the shortest distance between two nodes.
  //
  // This is essentially a BFS starting with @from and ending at @to, with the
  // exception that we don't traverse down control edges, and all edges are
  // considered to have distance 1.
  int shortestDistanceBetweenNodes(unsigned from, unsigned to) const;

  //=-------- Program data ---------=//

  // Complete set of all execution nodes.
  ExecNodeMap nodes;

  // Dynamic data dependence graph.
  Graph graph;

  // Line number mapping to function and label name. If there are multiple
  // source files, there could be multiple function/labels with the same line
  // number.
  labelmap_t labelmap;

  // Maps a label in an inlined function to the original function in which it
  // was written.
  inline_labelmap_t inline_labelmap;

  // Control flow nodes corresponding to loop boundaries.  This is populated by
  // the LoopUnrolling optimization.
  std::vector<DynLoopBound> loop_bounds;

  // Caller-callee function argument name mappings.
  CallArgMap call_arg_map;

  // Vertices to their corresponding node ids.
  VertexNameMap vertex_to_name;

  // Loop sampling information.
  LoopInfo loop_info;
};

#endif
