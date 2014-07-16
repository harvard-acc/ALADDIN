#include <boost/config.hpp>
#include <iostream>
#include <boost/graph/adjacency_list.hpp>

template <class Graph, class Edge, class Vertex>
struct check_edgeid {
  check_edgeid(Graph &g, std::map<Edge, int>& e_to_n) : g_(g), {}

  int operator()(Vertex parent, Vertex node) const
  {
    std::pair<Graph::edge_descriptor, bool> edge_pair = boost::edge(parent, node, g_);
    Graph::edge_descriptor edge = edge_pair.first;
    return get(boost::edge_name, g_, edge);
  }
  Graph& g_;
};

template <class Graph, class Vertex>
struct has_one_child {
  has_one_child(Graph &g) : g_(g) {}
  
  bool operator()(Vertex node) const
  {
  
  }
  Graph& g_;
};

template <class Graph>
struct isolated_nodes {
  isolated_nodes(Graph &g) : G(g) {}
  int operator()(std::vector<bool> &isolated)
  {
  
  }
  Graph& G;
};

template <class Graph>
struct initialized_num_of_children {
  initialized_num_of_children(Graph &g) : G(g) {}
  void operator()(std::vector<int> &num_of_children)
  {
  
  }
  Graph& G;
};

template <class Graph>
struct initialized_num_of_parents {
  initialized_num_of_parents(Graph &g) : G(g) {}
  unsigned operator()(std::vector<bool> &isolated, std::vector<int> &num_of_parents)
  {
  
  }
  Graph& G;
};
