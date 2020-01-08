#ifndef __TYPEDEFS_H__
#define __TYPEDEFS_H__

#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/topological_sort.hpp>
#include <unordered_map>
#include <unordered_set>

typedef uint64_t Addr;

// Typedefs for Boost::Graph.

namespace boost {
enum vertex_node_id_t { vertex_node_id };
BOOST_INSTALL_PROPERTY(vertex, node_id);
}

typedef boost::property<boost::vertex_node_id_t, unsigned> VertexProperty;
typedef boost::property<boost::edge_name_t, uint8_t> EdgeProperty;
typedef boost::adjacency_list<boost::setS,
                              boost::vecS,
                              boost::bidirectionalS,
                              VertexProperty,
                              EdgeProperty> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;
typedef boost::graph_traits<Graph>::vertex_iterator vertex_iter;
typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
typedef boost::graph_traits<Graph>::in_edge_iterator in_edge_iter;
typedef boost::graph_traits<Graph>::out_edge_iterator out_edge_iter;
typedef boost::property_map<Graph, boost::edge_name_t>::type EdgeNameMap;
typedef boost::property_map<Graph, boost::vertex_node_id_t>::type VertexNameMap;

// Other convenience typedefs.
class PartitionEntry;
class ExecNode;
namespace SrcTypes {
  class UniqueLabel;
  class DynamicLabel;
  class DynamicVariable;
};

typedef std::unordered_map<SrcTypes::UniqueLabel, unsigned> unrolling_config_t;
typedef std::unordered_set<SrcTypes::UniqueLabel> pipeline_config_t;
typedef std::unordered_map<std::string, PartitionEntry> partition_config_t;
typedef std::multimap<unsigned, SrcTypes::UniqueLabel> labelmap_t;
typedef std::unordered_map<SrcTypes::UniqueLabel, SrcTypes::UniqueLabel>
    inline_labelmap_t;
typedef std::pair<ExecNode*, ExecNode*> node_pair_t;
typedef std::pair<const ExecNode*, const ExecNode*> cnode_pair_t;
typedef std::map<unsigned, ExecNode*> ExecNodeMap;

#endif
