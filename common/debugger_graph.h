#ifndef _DEBUGGER_GRAPH_
#define _DEBUGGER_GRAPH_

#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <boost/graph/breadth_first_search.hpp>

#include "ExecNode.h"
#include "ScratchpadDatapath.h"
#include "typedefs.h"
#include "debugger_graph.h"

class bfs_finished : public std::exception {
  virtual const char* what() const throw() {
    return "BFS finished";
  }
};

class NodeVisitor : public boost::default_bfs_visitor {
 public:
  NodeVisitor(Graph* _subgraph,
              std::map<unsigned, Vertex>* _existing_nodes,
              ScratchpadDatapath* _acc,
              Vertex _root_vertex,
              unsigned maxnodes,
              unsigned* _num_nodes_visited)
      : new_graph(_subgraph), existing_nodes(_existing_nodes), acc(_acc),
        root_vertex(_root_vertex), max_nodes(maxnodes),
        num_nodes_visited(_num_nodes_visited) {}

  // Insert a vertex with node_id if we aren't above the max_nodes limit.
  Vertex insert_vertex(unsigned node_id) const {
    if (*num_nodes_visited >= max_nodes)
      throw bfs_finished();

    return add_vertex(VertexProperty(node_id), *new_graph);
  }

  // Add a vertex and all of its children to the graph.
  void discover_vertex(const Vertex& v, const Graph& g) const {
    unsigned node_id = get(boost::vertex_node_id, g, v);
    auto node_it = existing_nodes->find(node_id);
    Vertex new_vertex;
    if (node_it == existing_nodes->end()) {
      new_vertex = insert_vertex(node_id);
      existing_nodes->operator[](node_id) = new_vertex;
      (*num_nodes_visited)++;
    } else {
      new_vertex = existing_nodes->at(node_id);
    }

    // If this node is a branch/call, don't print its children (unless this
    // node is the root of the BFS). These nodes tend to have a lot of children.
    if (new_vertex != root_vertex) {
      ExecNode* node = acc->getNodeFromNodeId(node_id);
      if (node->is_branch_op() || node->is_call_op())
        return;
    }

    // Check if any of its parents is a branch/call whose children we ignored.
    // If so, add an edge for each parent we already saw.
    in_edge_iter in_edge_it, in_edge_end;
    for (boost::tie(in_edge_it, in_edge_end) = in_edges(v, g);
         in_edge_it != in_edge_end;
         ++in_edge_it) {
      Vertex source_vertex_orig = source(*in_edge_it, g);
      unsigned source_id = get(boost::vertex_node_id, g, source_vertex_orig);
      auto source_it = existing_nodes->find(source_id);
      if (source_it != existing_nodes->end()) {
        Vertex source_vertex_new = source_it->second;
        unsigned edge_weight = get(boost::edge_name, g, *in_edge_it);
        add_edge(source_vertex_new,
                 new_vertex,
                 EdgeProperty(edge_weight),
                 *new_graph);
      }
    }

    // Immediately add all of its outgoing edges to the graph.
    out_edge_iter out_edge_it, out_edge_end;
    for (boost::tie(out_edge_it, out_edge_end) = out_edges(v, g);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      Vertex target_vertex_orig = target(*out_edge_it, g);
      unsigned target_id = get(boost::vertex_node_id, g, target_vertex_orig);
      if (existing_nodes->find(target_id) == existing_nodes->end()) {
        Vertex target_vertex = insert_vertex(target_id);
        unsigned edge_weight = get(boost::edge_name, g, *out_edge_it);
        add_edge(
            new_vertex, target_vertex, EdgeProperty(edge_weight), *new_graph);
        existing_nodes->operator[](target_id) = target_vertex;
        (*num_nodes_visited)++;
      }
    }
  }

 private:
  std::map<unsigned, Vertex> *existing_nodes;
  Graph* new_graph;
  ScratchpadDatapath* acc;
  Vertex root_vertex;
  unsigned max_nodes;
  unsigned* num_nodes_visited;
};

#endif
