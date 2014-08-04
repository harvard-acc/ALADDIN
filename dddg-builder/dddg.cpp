#include "dddg.h"

gzFile dynamic_func_file;
gzFile microop_file;
gzFile instid_file;
gzFile line_num_file;
gzFile memory_trace;
gzFile getElementPtr_trace;

gzFile prevBasicBlock_trace;

dddg::dddg()
{
  num_of_reg_dep = 0;
  num_of_mem_dep = 0;
  num_of_instructions = -1;
  last_parameter = 0;
  prev_bblock = "-1";
}
int dddg::num_edges()
{
  return register_edge_table.size() + memory_edge_table.size();
}
int dddg::num_nodes()
{
  return num_of_instructions;
}
int dddg::num_of_register_dependency()
{
  return num_of_reg_dep;
}
int dddg::num_of_memory_dependency()
{
  return num_of_mem_dep;
}
void dddg::output_method_call_graph(string bench)
{
  string output_file_name(bench);
  output_file_name += "_method_call_graph";
  write_string_file(output_file_name, method_call_graph.size(),
    method_call_graph);
}
void dddg::output_dddg(string dddg_file, string edge_parid_file, 
    //string edge_varid_file,
   string edge_latency_file)
{
  ofstream dddg;
  gzFile edge_parid, edge_latency;
  //gzFile edge_parid, edge_varid, edge_latency;

  dddg.open(dddg_file.c_str());
  edge_parid = gzopen(edge_parid_file.c_str(), "w");
  //edge_varid = gzopen(edge_varid_file.c_str(), "w");
  edge_latency = gzopen(edge_latency_file.c_str(), "w");
  //write title
  dddg << "digraph DDDG {" << endl;
  for (int node_id = 0; node_id < num_of_instructions ; node_id++)
    dddg << node_id << endl;
  
  int edge_id = 0;
  for(auto it = register_edge_table.begin(); 
    it != register_edge_table.end(); ++it)
  {
    dddg << it->first << " -> " << it->second.sink_node << " [e_id = " << edge_id << "];" << endl;
    edge_id++;
    //gzprintf(edge_varid, "%s\n", it->second.var_id.c_str());
    gzprintf(edge_parid, "%d\n", it->second.par_id);
    gzprintf(edge_latency, "1\n");
  }
  //Memory Dependency
  for(auto it = memory_edge_table.begin();
    it != memory_edge_table.end(); ++it)
  {
    dddg << it->first << " -> " << it->second.sink_node << " [e_id = " << edge_id << "];" << endl;
    edge_id++;
    //gzprintf(edge_varid, "%s\n", it->second.var_id.c_str());
    gzprintf(edge_parid, "%d\n", it->second.par_id);
    gzprintf(edge_latency, "1\n");
  }
  dddg << "}" << endl;
  dddg.close();
  gzclose(edge_parid);
  //gzclose(edge_varid);
  gzclose(edge_latency);
}
void dddg::parse_instruction_line(string line)
{
  char curr_static_function[256];
  int microop;
  char instid[256], bblockid[256];
  int line_num;
  char comma;
  sscanf(line.c_str(), "%d,%[^,],%[^,],%[^,],%d\n", &line_num, curr_static_function, bblockid, instid, &microop);
  
  prev_microop = curr_microop;
  curr_microop = microop;

  
  curr_instid = instid;
  
  if (!active_method.empty())
  {
     char prev_static_function[256];
     unsigned prev_counts;
     char dash;
     sscanf(active_method.top().c_str(), "%[^-]-%u", prev_static_function, &prev_counts);
     if (strcmp(curr_static_function, prev_static_function) != 0)
     {
       auto func_it = function_counter.find(curr_static_function);
       if (func_it == function_counter.end())
       {
         function_counter.insert(make_pair(curr_static_function, 0));
         ostringstream oss;
         oss << curr_static_function << "-0" ;
         curr_dynamic_function = oss.str();
       }
       else
       {
         func_it->second++;
         ostringstream oss;
         oss << curr_static_function << "-" << func_it->second;
         curr_dynamic_function = oss.str();
       }
       active_method.push(curr_dynamic_function);
       
        // if prev inst is a IRCALL instruction
       if (prev_microop == LLVM_IR_Call)
       {
         assert(callee_function == curr_static_function);
         
         ostringstream oss;
         oss << num_of_instructions << "," << prev_static_function << "-" << prev_counts  << "," << active_method.top();
         method_call_graph.push_back(oss.str());
       }
     }
     else
     //the same as last method
     {
       //calling it self
       if (prev_microop == LLVM_IR_Call && callee_function == curr_static_function)
       { 
         //a new instantiation
         auto func_it = function_counter.find(curr_static_function);
         assert(func_it != function_counter.end());
         func_it->second++;
         ostringstream oss;
         oss << curr_static_function << "-" << func_it->second;
         curr_dynamic_function = oss.str();
         active_method.push(curr_dynamic_function);
       }
       else
         curr_dynamic_function = active_method.top();
     }
     if (microop == LLVM_IR_Ret)
       active_method.pop();
  }
  else
  {
    auto func_it = function_counter.find(curr_static_function);
    if (func_it != function_counter.end())
    {
      func_it->second++;
      ostringstream oss;
      oss << curr_static_function << "-" << func_it->second;
      curr_dynamic_function = oss.str();
    }
    else
    {
      function_counter.insert(make_pair(curr_static_function, 0));
      ostringstream oss;
      oss << curr_static_function << "-0" ;
      curr_dynamic_function = oss.str();
    }
    active_method.push(curr_dynamic_function);
  }
  if (microop == LLVM_IR_PHI)
    prev_bblock = curr_bblock;
  curr_bblock = bblockid;
  gzprintf(prevBasicBlock_trace, "%s\n", prev_bblock.c_str());
  gzprintf(dynamic_func_file, "%s\n", curr_dynamic_function.c_str());
  gzprintf(microop_file, "%d\n", curr_microop);
	if(num_of_instructions==6675) printf("!!!%d\n",curr_microop);

  gzprintf(instid_file, "%s\n", curr_instid.c_str());
  gzprintf(line_num_file, "%d\n", line_num);
  num_of_instructions++;
  last_parameter = 0;
}
void dddg::parse_parameter(string line, int param_tag)
{
  int size, value, is_reg;
  char label[256];
  sscanf(line.c_str(), "%d,%d,%d,%[^\n]\n", &size, &value, &is_reg, label);
  if (!last_parameter)
  {
    num_of_parameters = param_tag;
    if (curr_microop == LLVM_IR_Call)
      callee_function = label;
  }
  last_parameter = 1;
  if (is_reg)
  {
    char unique_reg_id[256];
    sprintf(unique_reg_id, "%s-%s", curr_dynamic_function.c_str(), label);
    //string unique_reg_id (label);
    //unique_reg_id += curr_dynamic_function;
    auto reg_it = register_last_written.find(unique_reg_id);
    if (reg_it != register_last_written.end())
    {
      edge_node_info tmp_edge;
      tmp_edge.sink_node = num_of_instructions;
      //tmp_edge.var_id = label;
      tmp_edge.par_id = param_tag;
      register_edge_table.insert(make_pair(reg_it->second, tmp_edge));
      num_of_reg_dep++;
    }
  }
  if (curr_microop == LLVM_IR_Load || curr_microop == LLVM_IR_Store || curr_microop == LLVM_IR_GetElementPtr)
  {
    parameter_value.push_back(value);
    parameter_size.push_back(size);
    parameter_label.push_back(label);
    unsigned psz = parameter_value.size();
    //last parameter
    if (param_tag == 1 && curr_microop == LLVM_IR_Load)
    {
      unsigned mem_address = parameter_value.back();
      unsigned mem_size = parameter_size.back();
      gzprintf(memory_trace, "%d,%u,%u\n", num_of_instructions, mem_address, mem_size);
      auto addr_it = address_last_written.find(mem_address);
      if (addr_it != address_last_written.end())
      {
        unsigned source_inst = addr_it->second;
        auto same_source_inst = memory_edge_table.equal_range(source_inst);
        bool edge_existed = 0;
        for (auto sink_it = same_source_inst.first; sink_it != same_source_inst.second; sink_it++)
        {
          if (sink_it->second.sink_node == num_of_instructions)
          {
            edge_existed = 1;
            break;
          }
        }
        if (!edge_existed)
        {
          edge_node_info tmp_edge;
          tmp_edge.sink_node = num_of_instructions;
          //tmp_edge.var_id = "";
          tmp_edge.par_id = -1;
          memory_edge_table.insert(make_pair(source_inst, tmp_edge));
          num_of_mem_dep++;
        }
      }
      unsigned base_address = parameter_value.back();
      string base_label = parameter_label.back();
      gzprintf(getElementPtr_trace, "%d,%s,%u\n", num_of_instructions, base_label.c_str(), base_address);
    }
    else if (param_tag == 2 && curr_microop == LLVM_IR_Store) //2nd of Store is the pointer while 1st is the value
    {
      unsigned mem_address = parameter_value[psz-1];
      unsigned mem_size = parameter_size[psz-1];


      gzprintf(memory_trace, "%d,%u,%u\n", num_of_instructions, mem_address, mem_size);
      auto addr_it = address_last_written.find(mem_address);
      if (addr_it != address_last_written.end())
        addr_it->second = num_of_instructions;
      else
        address_last_written.insert(make_pair(mem_address, num_of_instructions));
      unsigned base_address = parameter_value[psz-1];
      string base_label = parameter_label[psz-1];
      gzprintf(getElementPtr_trace, "%d,%s,%u\n", num_of_instructions, base_label.c_str(), base_address);
    }
    else if (param_tag == 1 && curr_microop == LLVM_IR_GetElementPtr)
    {
      unsigned base_address = parameter_value[psz-1];
      string base_label = parameter_label[psz-1];
      gzprintf(getElementPtr_trace, "%d,%s,%u\n", num_of_instructions, base_label.c_str(), base_address);
    }
  }
}

void dddg::parse_result(string line)
{
  int size, value, is_reg;
  char label[256];
  
  sscanf(line.c_str(), "%d,%d,%d,%[^\n]\n", &size, &value, &is_reg, label);
  
  assert(is_reg);
  
  char unique_reg_id[256];
  sprintf(unique_reg_id, "%s-%s", curr_dynamic_function.c_str(), label);
  auto reg_it = register_last_written.find(unique_reg_id);
  
  if (reg_it != register_last_written.end())
    reg_it->second = num_of_instructions;
  else
    register_last_written[unique_reg_id] = num_of_instructions;
}

int build_initial_dddg(string bench, string trace_file_name)
  //int build_initial_dddg(string bench, string trace_file_name, string config_file)
{
  if (!fileExists(trace_file_name))
  {
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << " ERROR: Input Trace Not Found  " << std::endl;
    std::cerr << "-------------------------------" << std::endl;
    return 1;
  }
	else
  {
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << "      Generating DDDG          " << std::endl;
    std::cerr << "-------------------------------" << std::endl;
  } 
	
  dddg graph_dep;

  FILE *tracefile;
  
  tracefile = fopen(trace_file_name.c_str(), "r");

  string func_file_name, microop_file_name, instid_file_name;
  string memory_trace_name, getElementPtr_trace_name;
  string resultVar_trace_name, line_num_file_name;
  string prevBasicBlock_trace_name;

  func_file_name = bench + "_dynamic_funcid.gz";
  microop_file_name = bench + "_microop.gz";
  instid_file_name = bench + "_instid.gz";
  line_num_file_name = bench + "_linenum.gz";
  memory_trace_name = bench + "_memaddr.gz";
  getElementPtr_trace_name = bench + "_getElementPtr.gz";
  prevBasicBlock_trace_name = bench + "_prevBasicBlock.gz";

  dynamic_func_file  = gzopen(func_file_name.c_str(), "w");
  microop_file = gzopen(microop_file_name.c_str(), "w");
	instid_file = gzopen(instid_file_name.c_str(), "w");
	line_num_file = gzopen(line_num_file_name.c_str(), "w");
  memory_trace = gzopen(memory_trace_name.c_str(), "w");
  getElementPtr_trace = gzopen(getElementPtr_trace_name.c_str(), "w");
  prevBasicBlock_trace = gzopen(prevBasicBlock_trace_name.c_str(), "w");

  
  char buffer[256];
  while(!feof(tracefile)) 
  {
    if (fgets(buffer, sizeof(buffer), tracefile) == NULL)
      continue;
    string wholeline(buffer); 
    size_t pos_end_tag = wholeline.find(","); 
    
    if(pos_end_tag == std::string::npos) { continue; }
    string tag = wholeline.substr(0,pos_end_tag); 
    string line_left = wholeline.substr(pos_end_tag + 1);
    if (tag.compare("0") == 0)
      graph_dep.parse_instruction_line(line_left); 
    else if (tag.compare("r")  == 0)
      graph_dep.parse_result(line_left);	
    else 
      graph_dep.parse_parameter(line_left, atoi(tag.c_str()));	
 }

  fclose(tracefile);
  
  gzclose(dynamic_func_file);
  gzclose(microop_file);
  gzclose(instid_file);
  gzclose(line_num_file);
  gzclose(memory_trace);
  gzclose(getElementPtr_trace);
  gzclose(prevBasicBlock_trace);
  
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "Num of Nodes: " << graph_dep.num_nodes() << std::endl;
  std::cerr << "Num of Edges: " << graph_dep.num_edges() << std::endl;
  std::cerr << "Num of Reg Edges: " << graph_dep.num_of_register_dependency() << std::endl;
  std::cerr << "Num of MEM Edges: " << graph_dep.num_of_memory_dependency() << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  string graph_file, edge_parid, edge_latency;
  //string graph_file, edge_varid, edge_parid, edge_latency;
  graph_file = bench + "_graph";
  //edge_varid = bench + "_edgevarid.gz";
  edge_parid = bench + "_edgeparid.gz";
  edge_latency = bench + "_edgelatency.gz";

  graph_dep.output_dddg(graph_file, edge_parid, edge_latency);
  graph_dep.output_method_call_graph(bench);
	
  return 0;

}

