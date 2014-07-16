#ifndef __DATAPATH__
#define __DATAPATH__

#include <boost/graph/graphviz.hpp>
#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <iostream>
#include <assert.h>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <list>
#include <set>
#include "file_func.h"
#include "opcode_func.h"
#include "generic_func.h"
#include "./Scratchpad.h"

using namespace std;
typedef boost::property < boost::vertex_name_t, int> VertexProperty;
typedef boost::property < boost::edge_name_t, int> EdgeProperty;
typedef boost::adjacency_list < boost::vecS, boost::vecS, boost::bidirectionalS, VertexProperty, EdgeProperty> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;
typedef boost::graph_traits<Graph>::vertex_iterator vertex_iter;
typedef boost::graph_traits<Graph>::edge_iterator edge_iter;
typedef boost::graph_traits<Graph>::in_edge_iterator in_edge_iter;
typedef boost::graph_traits<Graph>::out_edge_iterator out_edge_iter;
typedef boost::property_map<Graph, boost::edge_name_t>::type EdgeNameMap;
typedef boost::property_map<Graph, boost::vertex_name_t>::type VertexNameMap;
typedef boost::property_map<Graph, boost::vertex_index_t>::type VertexIndexMap;

typedef boost::property < boost::vertex_name_t, string> MethodVertexProperty;
typedef boost::property < boost::edge_name_t, int> MethodEdgeProperty;
typedef boost::adjacency_list < boost::vecS, boost::vecS, boost::bidirectionalS, MethodVertexProperty, MethodEdgeProperty> MethodGraph;
typedef boost::graph_traits<MethodGraph>::vertex_descriptor MethodVertex;
typedef boost::graph_traits<MethodGraph>::edge_descriptor MethodEdge;
typedef boost::graph_traits<MethodGraph>::vertex_iterator method_vertex_iter;
typedef boost::graph_traits<MethodGraph>::edge_iterator method_edge_iter;
typedef boost::graph_traits<MethodGraph>::in_edge_iterator method_in_edge_iter;
typedef boost::graph_traits<MethodGraph>::out_edge_iterator method_out_edge_iter;
typedef boost::property_map<MethodGraph, boost::edge_name_t>::type MethodEdgeNameMap;
typedef boost::property_map<MethodGraph, boost::vertex_name_t>::type MethodVertexNameMap;
typedef boost::property_map<MethodGraph, boost::vertex_index_t>::type MethodVertexIndexMap;

class Scratchpad;

struct partitionEntry
{
  string type;
  unsigned array_size; //num of elements
  unsigned part_factor;
};
struct regEntry
{
  int size;
  int reads;
  int writes;
};
struct callDep
{
  string caller;
  string callee;
  int callInstID;
};
struct newEdge
{
  int from;
  int to;
  int parid;
  unsigned latency;
};
struct edgeAtt
{
  int parid;
  unsigned latency;
};

class Datapath
{
 public:
  Datapath(string bench, float cycle_t);
  ~Datapath();
  void setGlobalGraph();
  void clearGlobalGraph();
  void globalOptimizationPass();
  void initBaseAddress();
  void optimizationPass();
  void cleanLeafNodes();
  void completePartition();
  void removeInductionDependence();
  void removePhiNodes();
  void memoryAmbiguation();
  void removeAddressCalculation();
  void removeBranchEdges();
  void addCallDependence();
  void methodGraphBuilder();
  void methodGraphSplitter();
  void nodeStrengthReduction();
  void loopFlatten();
  void loopUnrolling();
  void removeSharedLoads();
  void storeBuffer();
  void removeRepeatedStores();
  void treeHeightReduction();
  void scratchpadPartition();
  
  bool readUnrollingConfig(std::unordered_map<int, int > &unrolling_config);
  bool readFlattenConfig(std::unordered_set<int> &flatten_config);
  bool readPartitionConfig(std::unordered_map<std::string,
         partitionEntry> & partition_config);
  bool readCompletePartitionConfig(std::unordered_set<std::string> &config);

  /*void readGraph(igraph_t *g);*/
  /*void readMethodGraph(igraph_t *g);*/
  void readGraph(Graph &g);
  void readMethodGraph(MethodGraph &g);
  void initMicroop(std::vector<int> &microop);
  void dumpStats();
  void writeFinalLevel();
  void writeGlobalIsolated();
  void writePerCycleActivity();
  void writeRegStats();
  void updateRegStats();
  void writeMicroop(std::vector<int> &microop);
  void initMethodID(std::vector<int> &methodid);
  void initDynamicMethodID(std::vector<string> &methodid);
  void initInstID(std::vector<std::string> &instid);
  void initAddressAndSize(std::unordered_map<unsigned, pair<unsigned, unsigned> > &address);
  void initAddress(std::unordered_map<unsigned, unsigned> &address);
  void initLineNum(std::vector<int> &lineNum);
  void initEdgeParID(std::vector<int> &parid);
  void initEdgeLatency(std::vector<unsigned> &edge_latency);
  void writeEdgeLatency(std::vector<unsigned> &edge_latency);
  void initGetElementPtr(std::unordered_map<unsigned, pair<string, unsigned> > &get_element_ptr);

  int writeGraphWithIsolatedEdges(std::vector<bool> &to_remove_edges);
  int writeGraphWithNewEdges(std::vector<newEdge> &to_add_edges, int curr_num_of_edges);
  int writeGraphWithIsolatedNodes(std::unordered_set<unsigned> &to_remove_nodes);
  void writeGraphInMap(std::unordered_map<string, edgeAtt> &full_graph, string name);
  void initializeGraphInMap(std::unordered_map<string, edgeAtt> &full_graph);

  void setGraphName(std::string graph_name, int min);
  void setGraphForStepping(std::string graph_name);
  int clearGraph();
  bool step();
  void stepExecutedQueue();
  void updateChildren(unsigned node_id, float latencySoFar);
  int fireMemNodes();
  int fireNonMemNodes();
  void initReadyQueue();
  void addMemReadyNode( unsigned node_id);
  void addNonMemReadyNode( unsigned node_id);
  void setScratchpad(Scratchpad *spad);
  void updateGlobalIsolated();
  
  void findMinRankNodes(int &node1, int &node2, std::unordered_map<unsigned, unsigned> &rank_map);

 private:
  
  //global/whole datapath variables
  Scratchpad *scratchpad;
  std::vector<bool> globalIsolated;
  std::vector<int> newLevel;
  std::vector<regEntry> regStats;
  std::vector<int> microop;
  std::unordered_map<unsigned, pair<string, unsigned> > baseAddress;

  unsigned numTotalNodes;

  char* benchName;
  float cycleTime;
  //stateful states
  int cycle;
  int prevCycle;
  
  //local/per method variables for step(), may need to include new data
  //structure for optimization phase
  char* graphName;
  unsigned numGraphNodes;
  unsigned numGraphEdges;
  unsigned minNode;

  
  /*igraph_t *g;*/
  /*Graph global_graph_;*/
  Graph graph_;
  std::unordered_map<int, Vertex> nameToVertex;
  VertexNameMap vertexToName;

  std::vector<int> numParents;
  std::vector<bool> isolated;
  std::vector<int> edgeLatency;
  /*std::unordered_map<int, int> callLatency;*/
  
  //stateful states
  unsigned totalConnectedNodes;
  unsigned executedNodes;

  std::set<unsigned> memReadyQueue;
  std::set<unsigned> nonMemReadyQueue;
  std::vector<pair<unsigned, float> > executedQueue;

};


#endif
