#include "dddg.h"

dddg::dddg()
{
  num_of_reg_dep = 0;
  num_of_mem_dep = 0;
  num_of_instructions = -1;
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
void dddg::output_methodid(string bench)
{
  string output_file_name(bench);
  output_file_name += "_dynamic_methodid.gz";
  write_gzip_string_file(output_file_name, v_methodid.size(),
    v_methodid);
}
void dddg::output_microop(string bench)
{
  string output_file_name(bench);
  output_file_name += "_microop.gz";
  write_gzip_file(output_file_name, v_microop.size(), v_microop);
}
void dddg::output_method_call_graph(string bench)
{
  string output_file_name(bench);
  output_file_name += "_method_call_graph";
  write_string_file(output_file_name, method_call_graph.size(),
    method_call_graph);
}
void dddg::output_dddg(string dddg_file, string edge_parid_file, 
   string edge_varid_file,
   string edge_latency_file)
{
  ofstream dddg;
  gzFile edge_parid, edge_varid, edge_latency;

  dddg.open(dddg_file.c_str());
  edge_parid = gzopen(edge_parid_file.c_str(), "w");
  edge_varid = gzopen(edge_varid_file.c_str(), "w");
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
    gzprintf(edge_varid, "%d\n", it->second.var_id);
    gzprintf(edge_parid, "%d\n", it->second.par_id);
    gzprintf(edge_latency, "1\n");
  }
  //Memory Dependency
  for(auto it = memory_edge_table.begin();
    it != memory_edge_table.end(); ++it)
  {
    dddg << it->first << " -> " << it->second.sink_node << " [e_id = " << edge_id << "];" << endl;
    edge_id++;
    gzprintf(edge_varid, "%d\n", it->second.var_id);
    gzprintf(edge_parid, "%d\n", it->second.par_id);
    gzprintf(edge_latency, "1\n");
  }
  dddg << "}" << endl;
  dddg.close();
  gzclose(edge_parid);
  gzclose(edge_varid);
  gzclose(edge_latency);
}
void dddg::parse_instruction_line(string line)
{
  char curr_static_function[256];
  int microop, bblockid;
  char instid[256];
  char comma;
  
  sscanf(line.c_str(), "%[^,],%d,%[^,],%d\n", curr_function, &bblockid, instid, microop);
  
  prev_microop = curr_microop;
  curr_microop = microop;
  v_microop.push_back(microop);

  if (!active_method.empty())
  {
     char prev_static_function[256];
     unsigned prev_counts;
     char dash;
     
     sscanf(active_method.top().c_str(), "%[^-]-%u", prev_static_function, prev_counts;)

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
         //FIXME: define callee_function somewhere
         cerr << "calling," << callee_function << ",curr func," << curr_static_function << endl;
         assert(callee_function == curr_static_function);
         
         ostringstream oss;
         oss << num_of_instructions << "," << prev_static_function << "-" << prev_counts  << "," << curr_instid << "," << active_method.top();
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
  v_methodid.push_back(curr_dynamic_function);
  curr_instid = instid;
  num_of_instructions++;
}
void dddg::parse_parameter(string line, int param_tag)
{
  int size, value, is_reg;
  char label[256];
  
  sscanf(line.c_str(), "%d,%d,%b,%[^,]\n", &size, &value, &is_reg, label);
  if (is_reg)
  {
    string unique_reg_id (label);
    unique_reg_id += curr_dynamic_function;
    auto reg_it = register_last_written.find(unique_reg_id);
    if (reg_it != register_last_written.end())
    {
      edge_node_info tmp_edge;
      tmp_edge.sink_node = num_of_instructions;
      tmp_edge.var_id = label;
      tmp_edge.par_id = param_tag;
      register_edge_table.insert(make_pair(reg_it->second, tmp_edge));
      num_of_reg_dep++;
    }
  }
  if (curr_microop == LLVM_IR_Load && curr_microop == LLVM_IR_Store)
  {
    parameter_value.inster(parameter_value.begin(), value);
    //last parameter
    if (parag_tag == 1 && curr_microop == LLVM_IR_Load)
    {
      unsigned mem_address = parameter_value.at(0);
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
          tmp_edge.var_id = "";
          tmp_edge.par_id = -1;
          memory_edge_table.insert(make_pair(source_inst, tmp_edge));
          num_of_mem_dep++;
        }
      }
    }
    else if (parag_tag == 1 && curr_microop == LLVM_IR_Store)
    {
      unsigned mem_address = parameter_value.at(1);
      auto addr_it = address_last_written.find(mem_address);
      if (addr_it != address_last_written.end())
        addr_it->second = num_of_instructions;
      else
        address_last_written.insert(make_pair(mem_address, num_of_instructions));
    }
  }
}

void dddg::parse_result(string line)
{
  int size, value, is_reg;
  char label[256];
  
  sscanf(line.c_str(), "%d,%d,%b,%[^,]\n", &size, &value, &is_reg, label);
  
  assert(is_reg);
  string unique_reg_id (label);
  unique_reg_id += curr_dynamic_function;
  
  auto reg_it = register_last_written.find(unique_reg_id);
  if (reg_it != register_last_written.end())
    reg_it->second = num_of_instructions;
  else
    register_last_written[unique_reg_id] = num_of_instructions;

}
void dddg::parse_call_parameter(string line, int param_tag)
{
  unsigned next_method = (unsigned) parameter_value.at(0);
  auto func_it = function_counter.find(next_method);
  string next_method_id;
  if (func_it != function_counter.end())
  {
    int count = func_it->second;
    ostringstream oss;
    oss << next_method << "-" <<++count; 
    next_method_id = oss.str();
  }
  else
  {
    ostringstream oss;
    oss << next_method << "-0" ;
    next_method_id = oss.str();
  }
  int par_type;
  int pos_end_par_type = line.find(",");
  par_type = atoi(line.substr(0,pos_end_par_type).c_str());
  string rest_line;
  rest_line = line.substr(pos_end_par_type + 1);
  if (par_type == IROFFSET)
  {
    istringstream parser(rest_line, istringstream::in);
    int var_id, internal_type, value, size;
    char comma;
    parser >> var_id >> comma >> internal_type >> comma >> size >> comma 
           >> value;
    auto func_it = method_register_table.find(curr_dynamic_function);
    int original_source = num_of_instructions;
    if (func_it != method_register_table.end())
    {
      auto register_it = func_it->second.find(var_id);
      if (register_it != func_it->second.end())
        original_source = register_it->second;
    }
      func_it = method_register_table.find(next_method_id);
      if (func_it != method_register_table.end())
        func_it->second[param_tag - 5] = original_source;
      else
      {
        hash_uint_to_uint register_update_table_per_method;
        register_update_table_per_method[param_tag - 5] = original_source;
        method_register_table[next_method_id] = register_update_table_per_method;
      }
  }
}

int build_initial_dddg(string bench, string trace_file_name)
  //int build_initial_dddg(string bench, string trace_file_name, string config_file)
{
	dddg graph_dep;

  FILE *tracefile;
  tracefile = fopen(trace_file_name.c_str(), "r");

	if (!tracefile)
  {
		std::cerr << "file is not found" << std::endl;
		return 11;
	}
	else
  {
    std::cerr << "===================START GENERATING DDG=======================" 
      << std::endl; 
    std::cerr << "Building DDG Original reading trace from " 
      << trace_file_name << std::endl; 
  } 
  
  int lineid = 0;
  while(!feof(tracefile)) 
  {
    char buffer[256];
    fgets(tracefile, buffer, 256);
    string wholeline(buffer); 
    
    size_t pos_end_tag = wholeline.find(","); 
    
    if(pos_end_tag == std::string::npos) { continue; }
    char tag; 
    string line_left; 
    tag = wholeline.substr(0,pos_end_tag).c_str(); 
    line_left = wholeline.substr(pos_end_tag + 1);
    
    if (strcmp(tag, "0") == 0)
    {
      graph_dep.parse_instruction_line(line_left); 
      lineid++;
    }
    else if (strcmp(tag, "r")  == 0)
      graph_dep.parse_result(line_left);	
    else 
      //if (tag > 0 && tag < 4)
      graph_dep.parse_parameter(line_left, tag);	
    //else
      //graph_dep.parse_call_parameter(line_left, tag);	
 }

  fclose(tracefile);

  std::cerr << "num of nodes " << graph_dep.num_nodes() << std::endl; 
  std::cerr << "num of edges " << graph_dep.num_edges() << std::endl; 
  std::cerr << "num of reg edges " << graph_dep.num_of_register_dependency() << std::endl; 
  std::cerr << "num of mem edges " << graph_dep.num_of_memory_dependency() << std::endl; 

  string graph_file, edge_varid, edge_parid, edge_latency;
  graph_file = bench + "_graph";
  edge_varid = bench + "_edgevarid.gz";
  edge_parid = bench + "_edgeparid.gz";
  edge_latency = bench + "_edgelatency.gz";

  graph_dep.output_dddg(graph_file, edge_parid, edge_varid, edge_latency);
  graph_dep.output_method_call_graph(bench);
  graph_dep.output_methodid(bench);
  graph_dep.output_microop(bench);
  std::cerr << "=====================END GENERATING DDG====================== " << std::endl;
  std::cerr << endl;
	
  return 0;

}

