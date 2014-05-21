#ifndef DDDG_H
#define DDDG_H

#include "file_func.h"
#include "iljit_func.h"
#include <stack>

using namespace std;

struct edge_node_info{
  unsigned sink_node;
  int var_id;
  int par_id;
};


//data structure used to tract dependency
typedef unordered_map<unsigned int, unsigned int> hash_uint_to_uint;
typedef unordered_map<string, hash_uint_to_uint> nested_hash_string_to_uint;
typedef unordered_multimap<unsigned int, edge_node_info> hashmulti_uint_to_node_info;

typedef hash_uint_to_uint::iterator hash_uint_to_uint_it;
typedef hashmulti_uint_to_node_info::iterator hashmulti_uint_to_node_info_it;



class dddg
{
public:
  dddg();
  int num_edges();
  int num_nodes();
  int num_of_register_dependency();
  int num_of_memory_dependency();
  /*void init_to_ignore_method(string file_name);*/
  void output_methodid(string bench);
  void output_method_call_graph(string bench);
  void output_dddg(string dddg_file, string edge_parid_file, 
   string edge_varid_file,
   string edge_latency_file);
  void parse_instruction_line(string line);
  void parse_parameter(string line, int param_tag);
  void parse_result(string line);
  void parse_call_parameter(string line, int param_tag);
private:
  string current_method;
  unsigned current_microop;
  unsigned current_instid;
  vector<unsigned> parameter_value;
  vector<string> method_call_graph;
  vector<string> v_methodid;
  /*unordered_map<unsigned, bool> to_ignore_methodid;*/
  int num_of_instructions;
  int num_of_reg_dep;
  int num_of_mem_dep;

	//register dependency tracking table using hash_map(hash_map)
	nested_hash_string_to_uint method_register_table;
	//memory dependency tracking table
	hash_uint_to_uint address_dependency_table;
	//edge multimap
	hashmulti_uint_to_node_info register_edge_table;
	hashmulti_uint_to_node_info memory_edge_table;
	//keep track of currently executed methods
	stack<string> active_method;
	//manage methods
	hash_uint_to_uint method_appearance_table;
};

int build_initial_dddg(string bench, string trace_file_name);
#endif
