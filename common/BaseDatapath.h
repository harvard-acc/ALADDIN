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

#include "AladdinExceptions.h"
#include "ExecNode.h"
#include "typedefs.h"
#include "DDDG.h"
#include "file_func.h"
#include "opcode_func.h"
#include "MemoryType.h"
#include "Program.h"
#include "Registers.h"
#include "Scratchpad.h"
#include "SourceManager.h"
#include "DynamicEntity.h"
#include "user_config.h"

// Records the number of reads and writes from a memory block for a specific
// cycle in terms of bytes.
struct MemoryActivity {
  int bytes_read;
  int bytes_write;
};

struct FunctionActivity {
  unsigned mul;
  unsigned add;
  unsigned bit;
  unsigned shifter;
  unsigned fp_sp_mul;
  unsigned fp_dp_mul;
  unsigned fp_sp_add;
  unsigned fp_dp_add;
  unsigned trig;
  FunctionActivity() {
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

struct RegStatEntry {
  int reads;
  int writes;
};

class Scratchpad;

// Data to print out to file, stdout, or database.
struct summary_data_t {
  std::string benchName;
  std::string topLevelFunctionName;
  int num_cycles;
  int upsampled_cycles;
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
    const ExecNode* node = vertexToNode[v];
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

  //=----------- Program building functions ------------=//

  // Build the program: the DDDG and exec node data.
  //
  // Return true if the graph was built, false if not. False can happen if the
  // trace was empty.
  bool buildDddg();

  // Reset the dynamic trace to the beginning.
  virtual void resetTrace();

  // Add a function to the list of functions.
  void addFunctionName(std::string func_name) {
    functionNames.insert(func_name);
  }

  void addArrayBaseAddress(const std::string& label, Addr addr) {
    auto part_it = user_params.partition.find(label);
    // Add checking for zero to handle DMA operations where we only use
    // base_addr to find the label name.
    if (part_it != user_params.partition.end() && part_it->second.base_addr == 0) {
      part_it->second.base_addr = addr;
    }
  }

  void addEntryArrayDecl(const std::string& array_name,
                         Addr addr) {
    auto part_it = user_params.partition.find(array_name);
    if (part_it == user_params.partition.end()) {
      // If this array doesn't exist in the user-declared arrays, then it
      // belongs to the host, which makes it only accessible by DMA.
      PartitionEntry entry = { dma, none, 0, 0, 0, addr };
      user_params.partition[array_name] = entry;
    } else {
      // If we find an entry with a matching name, it was either a
      // user-specified array (which means it's part of the actual
      // accelerator), or it is a redeclaration of a host array. Either way,
      // update the base address.
      PartitionEntry& entry = part_it->second;
      entry.base_addr = addr;
    }
  }

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
  void initDmaBaseAddress();

  //=------------ Program access functions --------------=//

  std::string getBenchName() { return benchName; }
  SrcTypes::SourceManager& get_source_manager() { return srcManager; }
  const Program& getProgram() const { return program; }
  Addr getBaseAddress(const std::string& label) {
    auto it = user_params.partition.find(label);
    if (it == user_params.partition.end())
      throw UnknownArrayException(label);
    return it->second.base_addr;
  }

  //=----------- User configuration functions ------------=//

  bool isReadyMode() const { return user_params.ready_mode; }

  //=----------- Simulation/scheduling functions --------=//

  unsigned getCurrentCycle() { return num_cycles; }
  virtual void prepareForScheduling();
  virtual int rescheduleNodesWhenNeeded();
  void dumpGraph(std::string graph_name);
  void dumpStats();

  //=------------ Clean up functions -----------=//

  virtual void clearDatapath();
  void clearFunctionName() { functionNames.clear(); }
  void clearArrayBaseAddress() {
    // Don't clear user_params.partition - it only gets read once.
    for (auto part_it = user_params.partition.begin();
         part_it != user_params.partition.end();
         ++part_it)
      part_it->second.base_addr = 0;
  }
  void clearRegStats() { regStats.clear(); }

#ifdef USE_DB
  // Specify the experiment to be associated with this simulation. Calling this
  // method will result in the summary data being written to a datbaase when the
  // simulation is complete.
  void setExperimentParameters(std::string experiment_name);
#endif

 protected:
  //=------------ Pure virtual functions -------------=//

  // Execute all nodes in the current cycle.
  virtual void stepExecutingQueue() = 0;

  // Run all the graph optimizations in the required order.
  virtual void globalOptimizationPass() = 0;

  // Compute total memory block area.
  virtual double getTotalMemArea() = 0;

  // Compute average memory power.
  virtual void getAverageMemPower(unsigned int cycles,
                                  float* avg_power,
                                  float* avg_dynamic,
                                  float* avg_leak) = 0;

  // Return a list of all memory blocks (i.e. arrays).
  virtual void getMemoryBlocks(std::vector<std::string>& names) = 0;

  //=------------ Graph construction routines -------------=//

  // Create a graph optimization object.
  template <typename T>
  std::unique_ptr<T> getGraphOpt() {
    return std::unique_ptr<T>(new T(program, srcManager, user_params));
  }

  //=------------- User configuration routines -------------=//

  // Configuration parsing and handling.
  void parse_config(std::string& bench, std::string& config_file);

  // Update unrolling/pipelining directives with labelmap information.
  void updateUnrollingPipeliningWithLabelInfo();

  //=-------------- Simulation and scheduling ----------------=/

  // State initialization.
  virtual void initBaseAddress();

  // After marking a node as completed, update the state of its children.
  virtual void updateChildren(ExecNode* node);

  void upsampleLoops();

  // Compute the number of registers needed at each cycle.
  void computeRegStats();

  // Per cycle Scheduling.
  virtual bool step();
  void copyToExecutingQueue();
  void initExecutingQueue();
  virtual void markNodeStarted(ExecNode* node);
  void markNodeCompleted(std::list<ExecNode*>::iterator& executingQueuePos,
                         int& advance_to);

  // Used to track cycle level activity for components identified by name.
  template <typename StatType>
  using cycle_activity_map =
      std::unordered_map<std::string, std::vector<StatType>>;

  // Stats output.
  void writePerCycleActivity();
  void writeBaseAddress();
  // A method that derived datapaths can overridie to dump custom stats.
  virtual void writeOtherStats();
  void initPerCycleActivity(
      std::vector<std::string>& comp_partition_names,
      std::vector<std::string>& mem_partition_names,
      cycle_activity_map<MemoryActivity>& mem_activity,
      cycle_activity_map<FunctionActivity>& func_activity,
      std::unordered_map<std::string, FunctionActivity>& func_max_activity,
      int num_cycles);
  void updatePerCycleActivity(
      cycle_activity_map<MemoryActivity>& mem_activity,
      cycle_activity_map<FunctionActivity>& func_activity,
      std::unordered_map<std::string, FunctionActivity>& func_max_activity);
  void outputPerCycleActivity(
      std::vector<std::string>& comp_partition_names,
      std::vector<std::string>& mem_partition_names,
      cycle_activity_map<MemoryActivity>& mem_activity,
      cycle_activity_map<FunctionActivity>& func_activity,
      std::unordered_map<std::string, FunctionActivity>& func_max_activity);
  void writeSummary(std::ostream& outfile, summary_data_t& summary);

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

  std::string benchName;
  std::string topLevelFunctionName;
  int num_cycles;
  int upsampled_cycles;

  // True if we have done the upsampling of execution statistics.
  bool upsampled;

  // The accelerated program description.
  Program program;

  // All options that can be specified by the user are contained here.
  UserConfigParams user_params;

  SrcTypes::SourceManager srcManager;

  /* True if the summarized results should be stored to a database, false
   * otherwise */
  bool use_db;

  /* A string identifier for a set of related simulations. For storing results
   * into databases, this is required. */
  std::string experiment_name;

  // The final set of edge weights after all graph optimizations have been run.
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

  std::vector<RegStatEntry> regStats;
  std::unordered_set<std::string> functionNames;
  std::vector<DynLoopBound> loopBound;

  unsigned totalConnectedNodes;
  unsigned executedNodes;

  std::list<ExecNode*> executingQueue;
  std::list<ExecNode*> readyToExecuteQueue;

  // Dynamic trace file name.
  gzFile trace_file;
  size_t current_trace_off;
  size_t trace_size;
};

#endif
