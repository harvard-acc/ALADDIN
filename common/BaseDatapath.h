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
#include <memory>
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
#include "SourceManager.h"
#include "DynamicEntity.h"
#include "user_config.h"

#include "graph_analysis_utils.h"

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

struct regEntry {
  int size;
  int reads;
  int writes;
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

/* Custom graphviz label writer that outputs the node id and microop of the vertex.
 *
 * Create a map from Vertex to the ExecNode object and call
 * make_microop_label_writer() with this map. This lets us output as much
 * information about each node as we want in the graph.
 */
template <class Map, class Graph>
class microop_label_writer {
 public:
  microop_label_writer(Map _vertexToNode, Graph& _graph)
      : vertexToNode(_vertexToNode), graph(_graph) {}
  void operator()(std::ostream& out, const Vertex& v) {
    ExecNode* node = vertexToNode[v];
    out << "[label=\"" << node->get_node_id() << "\\n("
        << node->get_microop_name() << ")\"]";
  }

 private:
  Map vertexToNode;
  Graph graph;
};

template <class Map, class Graph>
inline microop_label_writer<Map, Graph> make_microop_label_writer(Map map, Graph graph) {
  return microop_label_writer<Map, Graph>(map, graph);
}

class BaseDatapath {
 protected:
   typedef SrcTypes::DynamicVariable DynamicVariable;

 public:
  BaseDatapath(std::string& bench,
               std::string& trace_file_name,
               std::string& _config_file);
  virtual ~BaseDatapath();

  // Build graph.
  //
  // Return true if the graph was built, false if not. False can happen if the
  // trace was empty.
  bool buildDddg();
  unsigned getCurrentCycle() { return num_cycles; }

  SrcTypes::SourceManager& get_source_manager() { return srcManager; }

  // Change graph.
  void addDddgEdge(unsigned int from, unsigned int to, uint8_t parid);
  ExecNode* insertNode(unsigned node_id, uint8_t microop);
  void setLabelMap(labelmap_t& _labelmap) { labelmap = _labelmap; }
  const labelmap_t& getLabelMap() { return labelmap; }
  SrcTypes::UniqueLabel getUniqueLabel(ExecNode* node);

  // XXX: DDDG needs these functions as well. Separate this dependency (maybe a
  // composition of call argument map)?
  void addCallArgumentMapping(DynamicVariable& callee_reg_id,
                              DynamicVariable& caller_reg_id);
  DynamicVariable getCallerRegID(DynamicVariable& reg_id);

  virtual void prepareForScheduling();
  virtual int rescheduleNodesWhenNeeded();
  void dumpGraph(std::string graph_name);

  bool isReadyMode() const { return user_params.ready_mode; }

  // Accessing graph stats.
  const Graph& getGraph() { return graph_; }
  std::string getBenchName() { return benchName; }
  int getNumOfNodes() { return boost::num_vertices(graph_); }
  int getNumOfEdges() { return boost::num_edges(graph_); }
  int getMicroop(unsigned int node_id) {
    return exec_nodes.at(node_id)->get_microop();
  }
  int getNumOfConnectedNodes(unsigned int node_id) {
    return boost::degree(exec_nodes.at(node_id)->get_vertex(), graph_);
  }
  ExecNodeMap::iterator node_begin() { return exec_nodes.begin(); }
  ExecNodeMap::iterator node_end() { return exec_nodes.end(); }
  // Return all nodes with dependencies originating from OR leaving this node.
  std::vector<unsigned> getConnectedNodes(unsigned int node_id);
  std::vector<unsigned> getParentNodes(unsigned int node_id);
  std::vector<unsigned> getChildNodes(unsigned int node_id);

  std::list<ExecNode*> getNodesOfMicroop(unsigned microop);

  // XXX: We already have this as a free-standing function. Remove.
  ExecNode* getNextNode(unsigned node_id);

  /* XXX: Make this a free-standing function.
   *
   * Return the bounding nodes of function invocations.
   *
   * For each pair, the first element is the Call node that invokes the given
   * function, and the second element is the Return node of this invocation.
   */
  std::list<node_pair_t> findFunctionBoundaries(SrcTypes::Function* func);

  // For unit tests.
  int getUnrolledLoopBoundary(unsigned int region_id) {
    return loopBound.at(region_id).node_id;
  }
  std::string getBaseAddressLabel(unsigned int node_id) {
    return exec_nodes.at(node_id)->get_array_label();
  }
  unsigned getPartitionIndex(unsigned int node_id) {
    return exec_nodes.at(node_id)->get_partition_index();
  }
  long long int getBaseAddress(std::string label) {
    return user_params.partition.at(label).base_addr;
  }
  // XXX: These have duplicates in the base graph opt class, but they are
  // heavily used by the unit tests and the debugger.
  bool doesEdgeExist(ExecNode* from, ExecNode* to) {
    if (from != nullptr && to != nullptr)
      return doesEdgeExistVertex(from->get_vertex(), to->get_vertex());
    return false;
  }
  bool doesEdgeExistVertex(Vertex from, Vertex to) {
    return edge(from, to, graph_).second;
  }
  // This is kept for unit testing reasons only.
  bool doesEdgeExist(unsigned int from, unsigned int to) {
    return doesEdgeExist(exec_nodes.at(from), exec_nodes.at(to));
  }
  bool doesNodeExist(unsigned int node_id) {
    return exec_nodes.find(node_id) != exec_nodes.end();
  }

  ExecNode* getNodeFromVertex(const Vertex& vertex) {
    unsigned node_id = vertexToName[vertex];
    return exec_nodes.at(node_id);
  }
  ExecNode* getNodeFromNodeId(unsigned node_id) { return exec_nodes.at(node_id); }
  partition_config_t::iterator getArrayConfigFromAddr(Addr base_addr);
  // XXX: Currently only used for unit tests.
  int shortestDistanceBetweenNodes(unsigned int from, unsigned int to);
  void dumpStats();

  /*Set graph stats*/
  void addArrayBaseAddress(std::string label, long long int addr) {
    auto part_it = user_params.partition.find(label);
    // Add checking for zero to handle DMA operations where we only use
    // base_addr to find the label name.
    if (part_it != user_params.partition.end() && part_it->second.base_addr == 0) {
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
    for (auto& node_pair : exec_nodes)
      delete node_pair.second;
    exec_nodes.clear();
  }
  void clearFunctionName() { functionNames.clear(); }
  void clearArrayBaseAddress() {
    // Don't clear user_params.partition - it only gets read once.
    for (auto part_it = user_params.partition.begin();
         part_it != user_params.partition.end();
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
  void nodeStrengthReduction();
  void loopFlatten();
  void loopUnrolling();
  void loopPipelining();
  void perLoopPipelining();
  void storeBuffer();
  void removeSharedLoads();
  void removeRepeatedStores();
  void treeHeightReduction();
  void fuseRegLoadStores();
  void fuseConsecutiveBranches();

#ifdef USE_DB
  // Specify the experiment to be associated with this simulation. Calling this
  // method will result in the summary data being written to a datbaase when the
  // simulation is complete.
  void setExperimentParameters(std::string experiment_name);
#endif

 protected:
  template <typename T>
  std::unique_ptr<T> getGraphOpt() {
    return std::unique_ptr<T>(new T(exec_nodes,
                                    graph_,
                                    loopBound,
                                    vertexToName,
                                    labelmap,
                                    srcManager,
                                    call_argument_map,
                                    user_params));
  }


  // Configuration parsing and handling.
  void parse_config(std::string& bench, std::string& config_file);

  // Update unrolling/pipelining directives with labelmap information.
  void updateUnrollingPipeliningWithLabelInfo(
      const inline_labelmap_t& inline_labelmap);

  // State initialization.
  virtual void initBaseAddress();
  void initMethodID(std::vector<int>& methodid);

  virtual void updateChildren(ExecNode* node);
  void updateRegStats();

  // Scheduling
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

  std::string benchName;
  int num_cycles;

  // All options that can be specified by the user are contained here.
  UserConfigParams user_params;

  SrcTypes::SourceManager srcManager;

  /* Stores the register name mapping between caller and callee functions. */
  call_arg_map_t call_argument_map;

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
  // ID of the first node.
  unsigned int beginNodeId;
  // ID of the last node + 1.
  unsigned int endNodeId;

  // Completely partitioned arrays.
  Registers registers;

  // Complete list of all execution nodes and their properties.
  ExecNodeMap exec_nodes;

  // Maps line numbers to labels.
  labelmap_t labelmap;
  std::vector<regEntry> regStats;
  std::unordered_set<std::string> functionNames;
  std::vector<DynLoopBound> loopBound;
  // Scheduling.
  unsigned totalConnectedNodes;
  unsigned executedNodes;
  std::list<ExecNode*> executingQueue;
  std::list<ExecNode*> readyToExecuteQueue;

  // Trace file name.
  gzFile trace_file;
  size_t current_trace_off;
  size_t trace_size;
};

#endif
