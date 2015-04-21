#ifndef __BASE_DATAPATH__
#define __BASE_DATAPATH__

/* Base class of all datapath types. Child classes must implement the following
 * abstract methods:
 *
 * globalOptimizationPass()
 * stepExecutingQueue()
 * getTotalMemArea()
 * getAverageMemPower()
 * writeConfiguration()
 *
 */

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
#include <map>
#include <list>
#include <set>
#include <stdint.h>

#include "DatabaseDeps.h"

#include "ExecNode.h"
#include "boost_typedefs.h"
#include "DDDG.h"
#include "file_func.h"
#include "opcode_func.h"
#include "generic_func.h"
#include "Registers.h"
#include "Scratchpad.h"

#define CONTROL_EDGE 11
#define PIPE_EDGE 12

using namespace std;
// Used heavily in reporting cycle-level statistics.
typedef std::unordered_map<std::string, std::vector<int>> activity_map;
typedef std::unordered_map<std::string, int> max_activity_map;

class Scratchpad;

struct partitionEntry {
  std::string type;
  unsigned array_size;  // num of bytes
  unsigned wordsize;    // in bytes
  unsigned part_factor;
};
struct regEntry {
  int size;
  int reads;
  int writes;
};
struct callDep {
  std::string caller;
  std::string callee;
  int callInstID;
};
struct newEdge {
  ExecNode* from;
  ExecNode* to;
  int parid;
};
struct RQEntry {
  unsigned node_id;
  mutable float latency_so_far;
  mutable bool valid;
};

struct RQEntryComp {
  bool operator()(const RQEntry& left, const RQEntry& right) const {
    return left.node_id < right.node_id;
  }
};

// Data to print out to file, stdout, or database.
struct summary_data_t {
  std::string benchName;
  int num_cycles;
  float avg_power;
  float avg_mem_power;
  float avg_fu_power;
  float avg_fu_dynamic_power;
  float fu_leakage_power;
  float avg_mem_dynamic_power;
  float mem_leakage_power;
  float total_area;
  float fu_area;
  float mem_area;
  int max_mul;
  int max_add;
  int max_bit;
  int max_shifter;
  int max_reg;
};

class BaseDatapath {
 public:
  BaseDatapath(std::string bench, string trace_file, string config_file);
  virtual ~BaseDatapath();

  // Change graph.
  void addDddgEdge(unsigned int from, unsigned int to, uint8_t parid);
  void insertNode(unsigned node_id, uint8_t microop);
  void setGlobalGraph();
  virtual void setGraphForStepping();
  virtual int rescheduleNodesWhenNeeded();
  void dumpGraph(std::string graph_name);

  // Accessing graph stats.
  std::string getBenchName() { return benchName; }
  int getNumOfNodes() { return boost::num_vertices(graph_); }
  int getNumOfEdges() { return boost::num_edges(graph_); }
  int getMicroop(unsigned int node_id) {
    return exec_nodes[node_id]->get_microop();
  }
  int getNumOfConnectedNodes(unsigned int node_id) {
    return boost::degree(exec_nodes[node_id]->get_vertex(), graph_);
  }
  int getUnrolledLoopBoundary(unsigned int region_id) {
    return loopBound.at(region_id);
  }
  std::string getBaseAddressLabel(unsigned int node_id) {
    return exec_nodes[node_id]->get_array_label();
  }
  long long int getBaseAddress(std::string label) {
    return arrayBaseAddress[label];
  }
  bool doesEdgeExist(ExecNode* from, ExecNode* to) {
    if (from != nullptr && to != nullptr)
      return doesEdgeExistVertex(from->get_vertex(), to->get_vertex());
    return false;
  }
  // This is kept for unit testing reasons only.
  bool doesEdgeExist(unsigned int from, unsigned int to) {
    return doesEdgeExist(exec_nodes[from], exec_nodes[to]);
  }
  ExecNode* getNodeFromVertex(Vertex& vertex) {
    unsigned node_id = vertexToName[vertex];
    return exec_nodes[node_id];
  }
  int shortestDistanceBetweenNodes(unsigned int from, unsigned int to);

  // Graph optimizations.
  void removeInductionDependence();
  void removePhiNodes();
  void memoryAmbiguation();
  void removeAddressCalculation();
  void removeBranchEdges();
  void nodeStrengthReduction();
  void loopFlatten();
  void loopUnrolling();
  void loopPipelining();
  void storeBuffer();
  void removeSharedLoads();
  void removeRepeatedStores();
  void treeHeightReduction();

#ifdef USE_DB
  // Specify the experiment to be associated with this simulation. Calling this
  // method will result in the summary data being written to a datbaase when the
  // simulation is complete.
  void setExperimentParameters(std::string experiment_name);
#endif

 protected:
  // Graph transformation helpers.
  void findMinRankNodes(ExecNode** node1,
                        ExecNode** node2,
                        std::map<ExecNode*, unsigned>& rank_map);
  void cleanLeafNodes();
  bool doesEdgeExistVertex(Vertex from, Vertex to) {
    return edge(from, to, graph_).second;
  }

  // Configuration parsing and handling.
  void parse_config(std::string bench, std::string config_file);
  bool readPipeliningConfig();
  bool readUnrollingConfig(std::unordered_map<int, int>& unrolling_config);
  bool readFlattenConfig(std::unordered_set<int>& flatten_config);
  bool readPartitionConfig(
      std::unordered_map<std::string, partitionEntry>& partition_config);
  bool readCompletePartitionConfig(
      std::unordered_map<std::string, unsigned>& config);

  // State initialization.
  virtual void initBaseAddress();
  void initMethodID(std::vector<int>& methodid);
  void initDynamicMethods(std::unordered_set<std::string>& functions);
  void initPrevBasicBlock(std::vector<std::string>& prevBasicBlock);
  void initInstID();
  void initAddress();
  void initLineNum();
  void initGetElementPtr(
      std::unordered_map<unsigned, pair<std::string, long long int>>&
          get_element_ptr);

  // Graph updates.
  void updateGraphWithIsolatedEdges(std::set<Edge>& to_remove_edges);
  void updateGraphWithNewEdges(std::vector<newEdge>& to_add_edges);
  void updateGraphWithIsolatedNodes(std::vector<unsigned>& to_remove_nodes);
  virtual void updateChildren(ExecNode* node);
  void updateRegStats();

  // Scheduling
  void addMemReadyNode(unsigned node_id, float latency_so_far);
  void addNonMemReadyNode(unsigned node_id, float latency_so_far);
  int fireMemNodes();
  int fireNonMemNodes();
  void copyToExecutingQueue();
  void initExecutingQueue();
  void markNodeCompleted(std::list<ExecNode*>::iterator& executingQueuePos,
                         int& advance_to);

  // Stats output.
  void writePerCycleActivity();
  void writeBaseAddress();
  // Writes microop, execution cycle, and isolated nodes.
  void writeOtherStats();
  void initPerCycleActivity(std::vector<std::string>& comp_partition_names,
                            std::vector<std::string>& spad_partition_names,
                            activity_map& ld_activity,
                            activity_map& st_activity,
                            activity_map& mul_activity,
                            activity_map& add_activity,
                            activity_map& bit_activity,
                            activity_map& shifter_activity,
                            max_activity_map& max_mul_per_function,
                            max_activity_map& max_add_per_function,
                            max_activity_map& max_bit_per_function,
                            max_activity_map& max_shifter_per_function,
                            int num_cycles);
  void updatePerCycleActivity(activity_map& ld_activity,
                              activity_map& st_activity,
                              activity_map& mul_activity,
                              activity_map& add_activity,
                              activity_map& bit_activity,
                              activity_map& shifter_activity,
                              max_activity_map& max_mul_per_function,
                              max_activity_map& max_add_per_function,
                              max_activity_map& max_bit_per_function,
                              max_activity_map& max_shifter_per_function);
  void outputPerCycleActivity(std::vector<std::string>& comp_partition_names,
                              std::vector<std::string>& spad_partition_names,
                              activity_map& ld_activity,
                              activity_map& st_activity,
                              activity_map& mul_activity,
                              activity_map& add_activity,
                              activity_map& bit_activity,
                              activity_map& shifter_activity,
                              max_activity_map& max_mul_per_function,
                              max_activity_map& max_add_per_function,
                              max_activity_map& max_bit_per_function,
                              max_activity_map& max_shifter_per_function);
  void writeSummary(std::ostream& outfile, summary_data_t& summary);

  // Memory structures.
  virtual double getTotalMemArea() = 0;
  virtual void getAverageMemPower(unsigned int cycles,
                                  float* avg_power,
                                  float* avg_dynamic,
                                  float* avg_leak) = 0;
  virtual void getMemoryBlocks(std::vector<std::string>& names) = 0;

#ifdef USE_DB
  /* Gets the experiment id for experiment_name. If this is a new
   * experiment_name that doesn't exist in the database, it creates a new unique
   * experiment id and inserts it and the name into the database. */
  int getExperimentId(sql::Connection* con);

  /* Writes the current simulation configuration parameters into the appropriate
   * database table. Since different accelerators have different configuration
   * parameters, this is left pure virtual. */
  virtual int writeConfiguration(sql::Connection* con) = 0;

  /* Returns the last auto_incremented column value. This is used to get the
   * newly inserted config.id field. */
  int getLastInsertId(sql::Connection* con);

  /* Extracts the base Aladdin configuration parameters from the config file. */
  void getCommonConfigParameters(int& unrolling_factor,
                                 bool& pipelining,
                                 int& partition_factor);

  void writeSummaryToDatabase(summary_data_t& summary);
#endif

  // Miscellaneous
  void tokenizeString(std::string input, std::vector<int>& tokenized_list);

  virtual bool step();
  virtual void dumpStats();
  virtual void stepExecutingQueue() = 0;
  virtual void globalOptimizationPass() = 0;

  char* benchName;
  int num_cycles;
  float cycleTime;
  std::string trace_file;
  std::string config_file;
  /* True if the summarized results should be stored to a database, false
   * otherwise */
  bool use_db;
  /* A string identifier for a set of related simulations. For storing results
   * into databases, this is required. */
  std::string experiment_name;
  // boost graph.
  Graph graph_;
  VertexNameMap vertexToName;
  EdgeNameMap edgeToParid;

  // Graph node and edge attributes.
  unsigned numTotalNodes;
  unsigned numTotalEdges;

  // Completely partitioned arrays.
  Registers registers;

  // Complete list of all execution nodes and their properties.
  std::map<unsigned int, ExecNode*> exec_nodes;

  std::vector<regEntry> regStats;
  std::unordered_map<std::string, long long int> arrayBaseAddress;
  std::unordered_set<std::string> functionNames;
  std::vector<int> loopBound;
  // Scheduling.
  unsigned totalConnectedNodes;
  unsigned executedNodes;
  std::list<ExecNode*> executingQueue;
  std::list<ExecNode*> readyToExecuteQueue;
};

#endif
