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
 */

#include <iostream>
#include <assert.h>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <map>
#include <list>
#include <set>
#include <stdint.h>
#include <unistd.h>

#include "DatabaseDeps.h"

#include "ExecNode.h"
#include "typedefs.h"
#include "DDDG.h"
#include "file_func.h"
#include "opcode_func.h"
#include "generic_func.h"
#include "Registers.h"
#include "Scratchpad.h"

#define CONTROL_EDGE 11
#define PIPE_EDGE 12

struct memActivity {
  unsigned read;
  unsigned write;
};
struct funcActivity {
  unsigned mul;
  unsigned add;
  unsigned bit;
  unsigned shifter;
  unsigned fp_sp_mul;
  unsigned fp_dp_mul;
  unsigned fp_sp_add;
  unsigned fp_dp_add;
  unsigned trig;
  funcActivity() {
    mul = 0;
    add = 0;
    bit = 0;
    shifter = 0;
    fp_sp_mul = 0;
    fp_dp_mul = 0;
    fp_sp_add = 0;
    fp_dp_add = 0;
    trig = 0;
  }

  bool is_idle() {
    return (mul == 0 && add == 0 && bit == 0 && shifter == 0 &&
            fp_sp_mul == 0 && fp_dp_mul == 0 && fp_dp_add == 0 && trig == 0);
  }
};

class Scratchpad;

struct partitionEntry {
  std::string type;
  unsigned array_size;  // num of bytes
  unsigned wordsize;    // in bytes
  unsigned part_factor;
  long long int base_addr;
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
  int idle_fu_cycles;
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
  int max_fp_sp_mul;
  int max_fp_dp_mul;
  int max_fp_sp_add;
  int max_fp_dp_add;
  int max_trig;
  int max_mul;
  int max_add;
  int max_bit;
  int max_shifter;
  int max_reg;
};

/* Custom graphviz label writer that outputs the micro-op of the vertex.
 *
 * Create a map from Vertex to microop and call make_microop_label_writer()
 * with this map. The micro-op is not a Boost property of the Vertex, which is
 * why the Boost-supplied label writer class is insufficient.
 */
template <class Map> class microop_label_writer {
 public:
  microop_label_writer(Map _vertexToMicroop)
      : vertexToMicroop(_vertexToMicroop) {}
  void operator()(std::ostream& out, const Vertex& v) {
    out << "[label=" << vertexToMicroop[v] << "]";
  }

 private:
  Map vertexToMicroop;
};

template <class Map>
inline microop_label_writer<Map> make_microop_label_writer(Map map) {
  return microop_label_writer<Map>(map);
}

class BaseDatapath {
 public:
  BaseDatapath(std::string bench,
               std::string trace_file_name,
               std::string config_file);
  virtual ~BaseDatapath();

  // Build graph.
  void buildDddg();

  // Change graph.
  void addDddgEdge(unsigned int from, unsigned int to, uint8_t parid);
  ExecNode* insertNode(unsigned node_id, uint8_t microop);
  void addCallArgumentMapping(std::string callee_reg_id,
                              std::string caller_reg_id);
  std::string getCallerRegID(std::string callee_func,
                             std::string reg_id);
  virtual void prepareForScheduling();
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
  unsigned getPartitionIndex(unsigned int node_id) {
    return exec_nodes[node_id]->get_partition_index();
  }
  long long int getBaseAddress(std::string label) {
    return partition_config[label].base_addr;
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
  ExecNode* getNodeFromNodeId(unsigned node_id) { return exec_nodes[node_id]; }
  std::string getArrayLabelFromAddr(Addr base_addr);
  int shortestDistanceBetweenNodes(unsigned int from, unsigned int to);
  void dumpStats();

  /*Set graph stats*/
  void addArrayBaseAddress(std::string label, long long int addr) {
    auto part_it = partition_config.find(label);
    // Add checking for zero to handle DMA operations where we only use
    // base_addr to find the label name.
    if (part_it != partition_config.end() && part_it->second.base_addr == 0) {
      part_it->second.base_addr = addr;
    }
  }
  /*Return true if the func_name is added;
    Return false if the func_name is already added.*/
  bool addFunctionName(std::string func_name) {
    if (functionNames.find(func_name) == functionNames.end()) {
      functionNames.insert(func_name);
      return true;
    }
    return false;
  }

  /* Clear graph stats. */
  virtual void clearDatapath();
  void clearExecNodes() {
    for (int i = 0; i < exec_nodes.size(); i++)
      delete exec_nodes[i];
    exec_nodes.clear();
  }
  void clearFunctionName() { functionNames.clear(); }
  void clearArrayBaseAddress() {
    for (auto part_it = partition_config.begin();
         part_it != partition_config.end();
         ++part_it)
      part_it->second.base_addr = 0;
  }
  void clearLoopBound() { loopBound.clear(); }
  void clearRegStats() { regStats.clear(); }

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

  // State initialization.
  virtual void initBaseAddress();
  void initMethodID(std::vector<int>& methodid);

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
  virtual void markNodeStarted(ExecNode* node);
  void markNodeCompleted(std::list<ExecNode*>::iterator& executingQueuePos,
                         int& advance_to);

  // Stats output.
  void writePerCycleActivity();
  void writeBaseAddress();
  // Writes microop, execution cycle, and isolated nodes.
  void writeOtherStats();
  void initPerCycleActivity(
      std::vector<std::string>& comp_partition_names,
      std::vector<std::string>& mem_partition_names,
      std::unordered_map<std::string, std::vector<memActivity>>& mem_activity,
      std::unordered_map<std::string, std::vector<funcActivity>>& func_activity,
      std::unordered_map<std::string, funcActivity>& func_max_activity,
      int num_cycles);
  void updatePerCycleActivity(
      std::unordered_map<std::string, std::vector<memActivity>>& mem_activity,
      std::unordered_map<std::string, std::vector<funcActivity>>& func_activity,
      std::unordered_map<std::string, funcActivity>& func_max_activity);
  void outputPerCycleActivity(
      std::vector<std::string>& comp_partition_names,
      std::vector<std::string>& mem_partition_names,
      std::unordered_map<std::string, std::vector<memActivity>>& mem_activity,
      std::unordered_map<std::string, std::vector<funcActivity>>& func_activity,
      std::unordered_map<std::string, funcActivity>& func_max_activity);
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
                                 bool& pipelining_factor,
                                 int& partition_factor);

  void writeSummaryToDatabase(summary_data_t& summary);
#endif

  virtual bool step();
  virtual void stepExecutingQueue() = 0;
  virtual void globalOptimizationPass() = 0;

  char* benchName;
  int num_cycles;
  float cycleTime;
  std::string config_file;

  bool pipelining;
  /* Unrolling and flattening share unrolling_config;
   * flatten if factor is zero.
   * it maps static-method-linenumber to unrolling factor. */
  std::unordered_map<std::string, int> unrolling_config;
  /* complete, block, cyclic, and cache partition share partition_config */
  std::unordered_map<std::string, partitionEntry> partition_config;

  /* Stores the register name mapping between caller and callee functions. */
  std::unordered_map<std::string, std::string> call_argument_map;

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
  std::unordered_set<std::string> functionNames;
  std::vector<int> loopBound;
  // Scheduling.
  unsigned totalConnectedNodes;
  unsigned executedNodes;
  std::list<ExecNode*> executingQueue;
  std::list<ExecNode*> readyToExecuteQueue;

  // Trace file.
  gzFile trace_file;

  // Scratchpad config.
  /* True if ready-bit Scratchpad is used. */
  bool ready_mode;
  /* Num of read/write ports per partition. */
  unsigned num_ports;
};

#endif
