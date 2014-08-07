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

#define CONTROL_EDGE 11
#define PIPE_EDGE 12
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

class Scratchpad;

struct partitionEntry
{
  std::string type;
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
  std::string caller;
  std::string callee;
  int callInstID;
};
struct newEdge
{
  unsigned from;
  unsigned to;
  int parid;
};
struct RQEntry
{
  unsigned node_id;
  mutable float latency_so_far;
  mutable bool valid;
};

struct RQEntryComp
{
  bool operator() (const RQEntry& left, const RQEntry &right) const  
  { return left.node_id < right.node_id; }
};
class Datapath
{
 public:
  Datapath(std::string bench, float cycle_t);
  ~Datapath();
  void setGlobalGraph();
  void clearGlobalGraph();
  void globalOptimizationPass();
  void initBaseAddress();
  void cleanLeafNodes();
  void completePartition();
  void removeInductionDependence();
  void removePhiNodes();
  void memoryAmbiguation();
  void removeAddressCalculation();
  void removeBranchEdges();
  void nodeStrengthReduction();
  void loopFlatten();
  void loopUnrolling();
  void loopPipelining();
  void removeSharedLoads();
  void storeBuffer();
  void removeRepeatedStores();
  void treeHeightReduction();
  void scratchpadPartition();
  void findMinRankNodes(unsigned &node1, unsigned &node2, std::unordered_map<unsigned, unsigned> &rank_map);

  bool readPipeliningConfig();
  bool readUnrollingConfig(std::unordered_map<int, int > &unrolling_config);
  bool readFlattenConfig(std::unordered_set<int> &flatten_config);
  bool readPartitionConfig(std::unordered_map<std::string,
         partitionEntry> & partition_config);
  bool readCompletePartitionConfig(std::unordered_map<std::string, unsigned> &config);

  /*void readGraph(igraph_t *g);*/
  void dumpStats();
  void writeFinalLevel();
  void writeGlobalIsolated();
  void writePerCycleActivity();
  void writeMicroop(std::vector<int> &microop);
  
  void readGraph(Graph &g);
  void initMicroop(std::vector<int> &microop);
  void updateRegStats();
  void initMethodID(std::vector<int> &methodid);
  void initDynamicMethodID(std::vector<std::string> &methodid);
  void initPrevBasicBlock(std::vector<std::string> &prevBasicBlock);
  void initInstID(std::vector<std::string> &instid);
  void initAddressAndSize(std::unordered_map<unsigned, pair<long long int, unsigned> > &address);
  void initAddress(std::unordered_map<unsigned, long long int> &address);
  void initLineNum(std::vector<int> &lineNum);
  void initEdgeParID(std::vector<int> &parid);
  void initGetElementPtr(std::unordered_map<unsigned, pair<std::string, long long int> > &get_element_ptr);

  int writeGraphWithIsolatedEdges(std::vector<bool> &to_remove_edges);
  int writeGraphWithNewEdges(std::vector<newEdge> &to_add_edges, int curr_num_of_edges);
  int writeGraphWithIsolatedNodes(std::unordered_set<unsigned> &to_remove_nodes);
  void writeGraphInMap(std::unordered_map<std::string, int> &full_graph, std::string name);
  void initializeGraphInMap(std::unordered_map<std::string, int> &full_graph);

  void setGraphForStepping();
  void setScratchpad(Scratchpad *spad);
  
  bool step();
  void stepExecutedQueue();
  void updateChildren(unsigned node_id, float latencySoFar);
  int fireMemNodes();
  int fireNonMemNodes();
  void initReadyQueue();
  void addMemReadyNode( unsigned node_id, float latency_so_far);
  void addNonMemReadyNode( unsigned node_id, float latency_so_far);
  int clearGraph();
  
 private:
  
  //global/whole datapath variables
  Scratchpad *scratchpad;
  std::vector<int> newLevel;
  std::vector<regEntry> regStats;
  std::vector<int> microop;
  std::unordered_map<unsigned, pair<std::string, long long int> > baseAddress;

  unsigned numTotalNodes;
  unsigned numTotalEdges;

  char* benchName;
  float cycleTime;
  //stateful states
  int cycle;
  
  //local/per method variables for step(), may need to include new data
  //structure for optimization phase
  char* graphName;
  
  /*igraph_t *g;*/
  /*Graph global_graph_;*/
  Graph graph_;
  std::unordered_map<int, Vertex> nameToVertex;
  VertexNameMap vertexToName;
  EdgeNameMap edgeToName;

  std::vector<int> numParents;
  std::vector<int> edgeParid;
  std::vector<float> latestParents;
  std::vector<bool> finalIsolated;
  std::vector<int> edgeLatency;
  
  std::unordered_set<std::string> dynamicMemoryOps;
  std::unordered_set<std::string> functionNames;
  
  //stateful states
  unsigned totalConnectedNodes;
  unsigned executedNodes;

  /*std::set<RQEntry, RQEntryComp> memReadyQueue;*/
  /*std::set<RQEntry, RQEntryComp> nonMemReadyQueue;*/
  /*std::vector<pair<unsigned, float> > executingQueue;*/


};


#endif
