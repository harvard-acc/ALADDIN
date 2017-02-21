#ifndef __DDDG_H__
#define __DDDG_H__

#include <map>
#include <set>
#include <stack>
#include <string>
#include <zlib.h>
#include <stdlib.h>
#include <sstream>

#include "ExecNode.h"
#include "file_func.h"
#include "opcode_func.h"
#include "SourceManager.h"

struct reg_edge_t {
  unsigned sink_node;
  int par_id;
};

// data structure used to track dependency
typedef std::unordered_map<std::string, unsigned int> string_to_uint;
typedef std::unordered_map<Addr, unsigned int> uint_to_uint;
typedef std::unordered_multimap<unsigned int, reg_edge_t>
    multi_uint_to_reg_edge;
typedef std::map<unsigned int, std::set<unsigned int>> map_uint_to_set;

class BaseDatapath;



class DDDG {
 public:
  // Indicates that we have reached the end of the trace.
  static const size_t END_OF_TRACE = std::numeric_limits<size_t>::max();

  DDDG(BaseDatapath* _datapath, gzFile& _trace_file);
  int num_edges();
  int num_nodes();
  int num_of_register_dependency();
  int num_of_memory_dependency();
  int num_of_control_dependency();
  void output_dddg();
  size_t build_initial_dddg(size_t trace_off, size_t trace_size);
  std::multimap<unsigned, UniqueLabel> get_labelmap() { return labelmap; }

 private:
  void parse_instruction_line(std::string line);
  void parse_parameter(std::string line, int param_tag);
  void parse_result(std::string line);
  void parse_forward(std::string line);
  void parse_call_parameter(std::string line, int param_tag);
  void parse_labelmap_line(std::string line);
  std::string parse_function_name(std::string line);
  bool is_function_returned(std::string line, std::string target_function);

  // Enforce RAW/WAW dependencies on this memory access.
  //
  // start_addr: Address of the memory access.
  // size: Number of bytes modified.
  // sink_node: The node corresponding to this memory access.
  void handle_post_write_dependency(Addr start_addr,
                                    size_t size,
                                    unsigned sink_node);
  void insert_control_dependence(unsigned source_node, unsigned dest_node);
  const Variable& get_array_real_var(const std::string& array_name);

  DynamicFunction curr_dynamic_function;

  uint8_t curr_microop;
  uint8_t prev_microop;
  std::string prev_bblock;
  std::string curr_bblock;
  ExecNode* curr_node;

  Function callee_function;
  DynamicFunction callee_dynamic_function;

  bool last_parameter;
  int num_of_parameters;
  // Used to track the instruction that initialize call function parameters
  int last_call_source;
  /* Unique register ID in the caller function. Used to create a mapping between
   * register IDs in caller and callee functions. */
  DynamicVariable unique_reg_in_caller_func;

  std::string curr_instid;
  std::vector<Addr> parameter_value_per_inst;
  std::vector<unsigned> parameter_size_per_inst;
  std::vector<std::string> parameter_label_per_inst;
  long num_of_instructions;
  int num_of_reg_dep;
  int num_of_mem_dep;
  int num_of_ctrl_dep;
  int last_dma_fence;

  BaseDatapath* datapath;
  std::string trace_file_name;
  gzFile& trace_file;

  // Register dependency tracking table.
  multi_uint_to_reg_edge register_edge_table;
  // Memory dependence tracking table.
  map_uint_to_set memory_edge_table;
  // Control edge tracking table.
  map_uint_to_set control_edge_table;
  // Line number mapping to function and label name. If there are multiple
  // source files, there could be multiple function/labels with the same line
  // number.
  std::multimap<unsigned, UniqueLabel> labelmap;
  // keep track of currently executed methods
  std::stack<DynamicFunction> active_method;
  // manage methods
  std::unordered_map<DynamicVariable, unsigned> register_last_written;
  uint_to_uint address_last_written;
  // DMA nodes that have been seen since the last DMA fence.
  std::list<unsigned> last_dma_nodes;
  // This points to the SourceManager object inside a BaseDatapath object.
  SourceManager& srcManager;
};

#endif
