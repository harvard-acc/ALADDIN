#ifndef TRACE_PROFILER_H
#define TRACE_PROFILER_H

#include "file_func.h"
#include "iljit_func.h"
#include <stack>

using namespace std;

class traceProfiler
{
public:
  traceProfiler();
  void parse_instruction_line(string line);
  void parse_parameter(string line, int param_tag);
  void parse_result(string line);
  void output_parvalue( string parvalue_file_name, int which_parid);
  void output_partype( string partype_file_name, int which_parid);
  void output_varid( string varid_file_name, int which_varid);
  void output_instid(string instid_file);
  void output_methodid(string methodid_file);
  void output_microop(string microop_file);
  void output_memory_trace(string memory_trace_file_name);

private:
	int current_inst_op;
	std::vector<int> parameter_value;
	std::vector<int> microop_vector;
	std::vector<unsigned> methodid_vector;
	std::vector<int> instid_vector;
	std::vector< std::vector<int> > varid_vector;
	std::vector< std::vector<string> > parvalue_vector;
	std::vector< std::vector<int> > partype_vector;
	int num_of_instructions;
	
	//store memory address and access bytes
	vector<unsigned> mem_address_vector;
	vector<unsigned> mem_address_times;
};

int profile_init_stats( string bench, string trace_file_name);

#endif
