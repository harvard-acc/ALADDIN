#ifndef __DDDG_H__
#define __DDDG_H__

#include <stack>
#include <string>
#include <zlib.h>
#include <stdlib.h>

#include "ExecNode.h"
#include "file_func.h"
#include "opcode_func.h"

struct edge_node_info {
  unsigned sink_node;
  int par_id;
};

// data structure used to track dependency
typedef std::unordered_map<std::string, unsigned int> string_to_uint;
typedef std::unordered_map<Addr, unsigned int> uint_to_uint;
typedef std::unordered_multimap<unsigned int, edge_node_info>
    multi_uint_to_node_info;

class BaseDatapath;

class DDDG {

 private:
  BaseDatapath* datapath;

 public:
  DDDG(BaseDatapath* _datapath);
  int num_edges();
  int num_nodes();
  int num_of_register_dependency();
  int num_of_memory_dependency();
  void output_dddg();
  bool build_initial_dddg(gzFile trace_file);

 private:
  void parse_instruction_line(std::string line);
  void parse_parameter(std::string line, int param_tag);
  void parse_result(std::string line);
  void parse_forward(std::string line);
  void parse_call_parameter(std::string line, int param_tag);
  std::string parse_function_name(std::string line);
  bool is_function_returned(std::string line, std::string target_function);

  std::string curr_dynamic_function;

  uint8_t curr_microop;
  uint8_t prev_microop;
  std::string prev_bblock;
  std::string curr_bblock;
  ExecNode* curr_node;

  std::string callee_function;
  std::string callee_dynamic_function;

  bool last_parameter;
  int num_of_parameters;
  // Used to track the instruction that initialize call function parameters
  int last_call_source;

  std::string curr_instid;
  std::vector<Addr> parameter_value_per_inst;
  std::vector<unsigned> parameter_size_per_inst;
  std::vector<std::string> parameter_label_per_inst;
  int num_of_instructions;
  int num_of_reg_dep;
  int num_of_mem_dep;

  // register dependency tracking table using hash_map(hash_map)
  // memory dependency tracking table
  // edge multimap
  multi_uint_to_node_info register_edge_table;
  multi_uint_to_node_info memory_edge_table;
  // keep track of currently executed methods
  std::stack<std::string> active_method;
  // manage methods
  string_to_uint function_counter;
  string_to_uint register_last_written;
  uint_to_uint address_last_written;
};

#endif
