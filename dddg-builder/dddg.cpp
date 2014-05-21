#include "dddg.h"

dddg::dddg()
{
  num_of_reg_dep = 0;
  num_of_mem_dep = 0;
  num_of_instructions = -1;
  current_microop = 0;
  parameter_value.push_back(0) ;
  parameter_value.push_back(0) ;
  parameter_value.push_back(0) ;
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
//void dddg::init_to_ignore_method(string file_name)
//{
  //ifstream input;
  //input.open(file_name);
  //while (!input.eof())
  //{
    //string wholeline;
    //getline(input, wholeline);
    //if (wholeline.size() == 0)
      //break;
    //unsigned pos = wholeline.find("ignore,");
    //if (pos == std::string::npos)
      //continue;
    //else
    //{
      //int methodid = atoi(wholeline.substr(pos+1).c_str());
      //to_ignore_methodid[method_id] = 1;
    //}
  //}
  //input.close();
//}
void dddg::output_methodid(string bench)
{
  string output_file_name(bench);
  output_file_name += "_dynamic_methodid.gz";
  write_gzip_string_file(output_file_name, v_methodid.size(),
    v_methodid);
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
  ogzstream edge_parid, edge_varid, edge_latency;

  dddg.open(dddg_file.c_str());
  edge_parid.open(edge_parid_file.c_str());
  edge_varid.open(edge_varid_file.c_str());
  edge_latency.open(edge_latency_file.c_str());

  //Register Dependency
  for(auto it = register_edge_table.begin(); 
    it != register_edge_table.end(); ++it)
  {
    dddg << it->first << " " << it->second.sink_node << endl;
    edge_varid << it->second.var_id << endl;
    edge_parid << it->second.par_id << endl;
    edge_latency << "1" << endl;
  }
  //Memory Dependency
  for(auto it = memory_edge_table.begin();
    it != memory_edge_table.end(); ++it)
  {
    dddg << it->first << " " << it->second.sink_node << endl;
    edge_varid << it->second.var_id << endl;
    edge_parid << it->second.par_id << endl;
    edge_latency << "1" << endl;
  }
  dddg.close();
  edge_parid.close();
  edge_varid.close();
  edge_latency.close();
}
void dddg::parse_instruction_line(string line)
{
  unsigned node_method;
  unsigned instid, microop, bblockid;
  char comma;
  istringstream parser(line, istringstream::in);
  parser >> node_method >> comma >> bblockid >> comma >> instid >> comma >> microop;

  if (!active_method.empty())
  {
     unsigned last_method_id, counts;
     char dash;
     istringstream top_stack_method(active_method.top(), istringstream::in);
     top_stack_method >> last_method_id >> dash >> counts;

     if (node_method != last_method_id)
     {
       auto method_it = method_appearance_table.find(node_method);
       if (method_it == method_appearance_table.end())
       {
         method_appearance_table.insert(make_pair(node_method, 0));
         ostringstream oss;
         oss << node_method << "-0" ;
         current_method = oss.str();
       }
       else
       {
         method_it->second++;
         ostringstream oss;
         oss << node_method << "-" << method_it->second;
         current_method = oss.str();
       }
       active_method.push(current_method);
       
        // FIXME
        // if prev inst is a IRCALL instruction
       if (current_microop == IRCALL)
       {
         unsigned calling_method = parameter_value.at(0);
         //cerr << "calling," << calling_method << ",node-method," << node_method << endl;
         assert(calling_method == node_method);
         auto method_it = method_appearance_table.find(calling_method);
         assert (method_it != method_appearance_table.end());
         
         string next_method_id;
         int count = method_it->second;
         ostringstream oss1;
         oss1 << calling_method << "-" <<count; 
         next_method_id = oss1.str();
         
         ostringstream oss;
         oss << num_of_instructions << "," << last_method_id << "-" << counts  << "," << current_instid << "," << next_method_id;
         method_call_graph.push_back(oss.str());
       }
     }
     else
     //the same as last method
     {
       if (instid == 0)
       { 
         //a new instantiation
         auto method_it = method_appearance_table.find(node_method);
         assert(method_it != method_appearance_table.end());
         method_it->second++;
         ostringstream oss;
         oss << node_method << "-" << method_it->second;
         current_method = oss.str();
         active_method.push(current_method);
       }
       else
         current_method = active_method.top();
     }
     if (microop == IRRET)
       active_method.pop();
  }
  else
  {
    auto method_it = method_appearance_table.find(node_method);
    if (method_it != method_appearance_table.end())
    {
      method_it->second++;
      ostringstream oss;
      oss << node_method << "-" << method_it->second;
      current_method = oss.str();
    }
    else
    {
      method_appearance_table.insert(make_pair(node_method, 0));
      ostringstream oss;
      oss << node_method << "-0" ;
      current_method = oss.str();
    }
    active_method.push(current_method);
  }
  v_methodid.push_back(current_method);
  current_microop = microop;
  current_instid = instid;
  num_of_instructions++;
}
void dddg::parse_parameter(string line, int param_tag)
{
  int par_type;
  int pos_end_par_type = line.find(",");
  par_type = atoi(line.substr(0,pos_end_par_type).c_str());
  string rest_line;
  rest_line = line.substr(pos_end_par_type + 1);
  
  if (par_type == IROFFSET)
  {
    istringstream parser(rest_line, istringstream::in);
    int var_id, internal_type, size;
    unsigned value;
    char comma;
    parser >> var_id >> comma >> internal_type >> comma >> size >> comma 
           >> value;
    parameter_value.at(param_tag-1) = (unsigned)value;
    auto method_it = method_register_table.find(current_method);
    if (method_it != method_register_table.end())
    {
      auto register_it = (method_it->second).find(var_id);
      if (register_it != method_it->second.end())
      {
        edge_node_info temp_edge;
        temp_edge.sink_node = (unsigned)num_of_instructions;
        temp_edge.var_id = var_id;
        temp_edge.par_id = param_tag;
        register_edge_table.insert(make_pair(register_it->second, temp_edge));
        num_of_reg_dep++;
      }
    }
  }
  else
  {
    istringstream parser(rest_line, istringstream::in);
    unsigned value, size;
    char comma;
    parser >> size >> comma >> value;
    parameter_value.at(param_tag-1) = (unsigned) value;
  }
  
  //Finish parsing the general cases, now specifically for memory ops
  if (param_tag == 2)
  {
    if (current_microop == IRSTOREREL)
    {
      unsigned mem_address = parameter_value.at(0) + parameter_value.at(1);
      auto address_it = address_dependency_table.find(mem_address);
      if (address_it != address_dependency_table.end())
        address_it->second = (unsigned)num_of_instructions;
      else
        address_dependency_table.insert(make_pair(mem_address,(unsigned)num_of_instructions));
    }
    else if (current_microop == IRLOADREL)
    {
      unsigned mem_address = parameter_value.at(0) + parameter_value.at(1);
      auto address_it = address_dependency_table.find(mem_address);
      if (address_it != address_dependency_table.end())
      {
        unsigned source_inst = address_it->second;
        auto same_source_nodes = memory_edge_table.equal_range(source_inst);
        bool edge_existed = 0;
        for (auto each_sink_node = same_source_nodes.first; 
          each_sink_node != same_source_nodes.second; each_sink_node++)
        {
          if (each_sink_node->second.sink_node == (unsigned)num_of_instructions)
          {
            edge_existed = 1;
            break;
          }
        }
        if (!edge_existed)
        {
          edge_node_info temp_edge;
          temp_edge.sink_node = (unsigned)num_of_instructions;
          temp_edge.var_id = -1;
          temp_edge.par_id = -1;
          memory_edge_table.insert(make_pair(source_inst, temp_edge));
          num_of_mem_dep++;
        }
      }
    }
  }
}

void dddg::parse_result(string line)
{
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
    auto method_it = method_register_table.find(current_method);
    if (method_it != method_register_table.end())
      (method_it->second)[var_id] = (unsigned)num_of_instructions;
    else
    {
      hash_uint_to_uint register_update_table_per_method;
      register_update_table_per_method[var_id] = (unsigned)num_of_instructions;
      method_register_table[current_method] = register_update_table_per_method;
    }
  }
  /*
  else
  {
    istringstream parser(rest_line, istringstream::in);
    int value;
    char comma;
    parser >> size >> comma >> value;
  }
  */

  //finish the general cases
  /*
  if (current_microop == IRRET && !(active_method.empty()))
  {
    string next_method_id = active_method.top();
    auto method_it = method_register_table.find(next_method_id);
    if (method_it != method_register_table.end())
      (method_it->second)[var_id] = num_of_instructions;
    else
    {
      hash_uint_to_uint register_update_table_per_method;
      register_update_table_per_method[var_id] = num_of_instructions;
      method_register_table[next_method_id] = register_update_table_per_method;
    }
  }
  */
}
void dddg::parse_call_parameter(string line, int param_tag)
{
  unsigned next_method = (unsigned) parameter_value.at(0);
  auto method_it = method_appearance_table.find(next_method);
  string next_method_id;
  if (method_it != method_appearance_table.end())
  {
    int count = method_it->second;
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
    auto method_it = method_register_table.find(current_method);
    int original_source = num_of_instructions;
    if (method_it != method_register_table.end())
    {
      auto register_it = method_it->second.find(var_id);
      if (register_it != method_it->second.end())
        original_source = register_it->second;
    }
      method_it = method_register_table.find(next_method_id);
      if (method_it != method_register_table.end())
        method_it->second[param_tag - 5] = original_source;
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
  //graph_dep.init_to_ignore_method(config);

	igzstream 			tracefile;
	tracefile.open(trace_file_name.c_str());

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
  
  string wholeline; 
  int lineid = 0;
  while(!tracefile.eof()) 
  {
    wholeline.clear(); 
    getline(tracefile, wholeline);
    size_t pos_end_tag = wholeline.find(","); 
    
    if(pos_end_tag == std::string::npos) { continue; }
    unsigned int tag; 
    string line_left; 
    tag = atoi(wholeline.substr(0,pos_end_tag).c_str()); 
    line_left = wholeline.substr(pos_end_tag + 1);
    
    if (tag == 0)
    {
      graph_dep.parse_instruction_line(line_left); 
      lineid++;
    }
    else if (tag > 0 && tag < 4)
      graph_dep.parse_parameter(line_left, tag);	
    else if (tag == 4)
      graph_dep.parse_result(line_left);	
    else
      graph_dep.parse_call_parameter(line_left, tag);	
 }

  tracefile.close();

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
  std::cerr << "=====================END GENERATING DDG====================== " << std::endl;
  std::cerr << endl;
	
  return 0;

}

