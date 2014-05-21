#include "traceProfiler.h"

traceProfiler::traceProfiler()
{
  num_of_instructions = -1;
  parameter_value.push_back(0) ;
  parameter_value.push_back(0) ;
  parameter_value.push_back(0) ;
}

void traceProfiler::output_memory_trace(string bench )
{
  ogzstream memory_trace_file;
  ostringstream oss;
  oss << bench << "_memaddr.gz";
  memory_trace_file.open(oss.str().c_str());
  for(unsigned int i = 0; i < mem_address_vector.size(); ++i)
    memory_trace_file << mem_address_vector.at(i) << "," << mem_address_times.at(i) << endl;
  memory_trace_file.close();
}

void traceProfiler::output_microop(string bench)
{
  ostringstream oss;
  oss << bench << "_microop.gz";
  write_gzip_file(oss.str(), microop_vector.size(), microop_vector);
}

void traceProfiler::output_methodid(string bench)
{
  ostringstream oss;
  oss << bench << "_methodid.gz";
  write_gzip_unsigned_file(oss.str(), methodid_vector.size(), methodid_vector);
}

void traceProfiler::output_instid(string bench)
{
  ostringstream oss;
  oss << bench << "_instid.gz";
  write_gzip_file(oss.str(), instid_vector.size(), instid_vector);
}

void traceProfiler::output_varid( string bench, int which_varid)
{
  ostringstream oss;
  oss << bench << "_par" << which_varid << "vid.gz";
  ogzstream varid_file;
  varid_file.open(oss.str().c_str());
  for(unsigned i = 0; i < varid_vector.size(); ++i)
    varid_file << varid_vector.at(i)[which_varid-1] << endl;
  varid_file.close();
}

void traceProfiler::output_parvalue( string bench, int which_parid)
{
  ostringstream oss;
  oss << bench << "_par" << which_parid << "value.gz";
  ogzstream parvalue_file;
  parvalue_file.open(oss.str().c_str());
  for(unsigned i = 0; i < parvalue_vector.size(); ++i)
    parvalue_file << parvalue_vector.at(i)[which_parid-1] << endl;
  parvalue_file.close();
}

void traceProfiler::output_partype( string bench, int which_parid)
{
  ostringstream oss;
  oss << bench << "_par" << which_parid << "type.gz";
  ogzstream partype_file;
  partype_file.open(oss.str().c_str());
  for(unsigned i = 0; i < partype_vector.size(); ++i)
    partype_file << partype_vector.at(i)[which_parid-1] << endl;
  partype_file.close();
}

void traceProfiler::parse_instruction_line(string line)
{
  unsigned method, inst_id, inst_op, bblockid;
  char comma;
  istringstream parser(line, istringstream::in);
  parser >> method >> comma >> bblockid >> comma >> inst_id >> comma >> inst_op;

  methodid_vector.push_back(method);
  microop_vector.push_back(inst_op);
  instid_vector.push_back(inst_id);

  mem_address_vector.push_back(0);
  mem_address_times.push_back(0);

  std::vector<int> temp_varid(4, -1);
  varid_vector.push_back(temp_varid);

  std::vector<string> temp_parvalue(4);
  parvalue_vector.push_back(temp_parvalue);

  std::vector<int> temp_partype(4,-1);
  partype_vector.push_back(temp_partype);

  current_inst_op = inst_op;
}

void traceProfiler::parse_parameter(string line, int param_tag)
{
  int par_type;
  string rest_line;
  int pos_end_par_type = line.find(",");
  
  par_type = atoi(line.substr(0, pos_end_par_type).c_str());
  rest_line = line.substr(pos_end_par_type + 1);
  int size;
  
  if (par_type == IROFFSET)
  {
    istringstream second_parser(rest_line, istringstream::in);
    int var_id, internal_type;
    string value;
    char comma;
    second_parser >> var_id >> comma >> internal_type >> comma >> size >> comma >> value;
    
    varid_vector.at( varid_vector.size() -1 )[param_tag -1] = var_id;
    parameter_value.at(param_tag-1) = atoi(value.c_str());
    parvalue_vector.at(parvalue_vector.size() -1 )[param_tag -1] = value;
    partype_vector.at(partype_vector.size() - 1)[param_tag - 1] =
      internal_type;
  }
  else
  {
    istringstream second_parser(rest_line, istringstream::in);
    string value;
    char comma;
    second_parser >> size >> comma >> value;
    parameter_value.at(param_tag-1) = atoi(value.c_str());
    parvalue_vector.at(parvalue_vector.size() -1 )[param_tag -1] = value;
    partype_vector.at(partype_vector.size() -1 )[param_tag -1] = par_type;
  }
  if (param_tag == 2 &&  current_inst_op == IRSTOREREL )
  {

    unsigned mem_address = parameter_value.at(0) + parameter_value.at(1);
    int temp_size = mem_address_vector.size();
    mem_address_vector[temp_size-1] = mem_address;
  }
  if (param_tag == 3 && current_inst_op == IRSTOREREL)
  {
    int temp_size = mem_address_vector.size();
    mem_address_times[temp_size-1] = size;
  }
  if (param_tag == 2 && current_inst_op == IRLOADREL )
  {
    unsigned mem_address = parameter_value.at(0) + parameter_value.at(1);
    int temp_size = mem_address_vector.size();
    mem_address_vector[temp_size-1] = mem_address;
  }
}

void traceProfiler::parse_result(string line)
{
  int par_type;
  string rest_line;
  int pos_end_par_type = line.find(",");
  
  par_type = atoi(line.substr(0, pos_end_par_type).c_str());
  rest_line = line.substr(pos_end_par_type + 1);
  
  int var_id, internal_type, size;
  string value;
  
  if (par_type == IROFFSET)
  {
    istringstream second_parser(rest_line, istringstream::in );
    char comma;
    second_parser >> var_id >> comma >> internal_type >> comma >> size >> comma >> value;
    
    varid_vector.at( varid_vector.size() -1 )[3] = var_id;
    parvalue_vector.at( parvalue_vector.size() -1 )[3] = value; 
    partype_vector.at( partype_vector.size() -1 )[3] = internal_type; 
  }
  else{
    istringstream second_parser(rest_line, istringstream::in);
    char comma;
    second_parser >> size >> comma >> value;
    parvalue_vector.at( parvalue_vector.size() -1 )[3] = value; 
    partype_vector.at( partype_vector.size() -1 )[3] = par_type; 
  }
  if (current_inst_op == IRLOADREL)
  {
    int temp_size = mem_address_vector.size();
    mem_address_times[temp_size-1] = size;
  }
}

int profile_init_stats( string bench, string trace_file_name)
{

	traceProfiler graph_dep;

	igzstream 			tracefile;
	tracefile.open(trace_file_name.c_str());

	if (!tracefile)
  {
		std::cerr << "file is not found" << std::endl;
		return 11;
	}
	else
  {
    std::cerr << "===================START PROFILING STATS=======================" 
      << std::endl; 
    std::cerr << "Reading trace from " 
      << trace_file_name << std::endl; 
  } 
  
  string wholeline; 
  while(!tracefile.eof()) 
  {
    wholeline.clear(); 
    getline(tracefile, wholeline);
		
    size_t  pos_end_tag = wholeline.find(","); 
    
    if(pos_end_tag == std::string::npos) { continue; }

    int tag; 
    string line_left; 
    tag = atoi(wholeline.substr(0,pos_end_tag).c_str()); 
    line_left = wholeline.substr(pos_end_tag + 1);
    
    if (tag == 0)
      graph_dep.parse_instruction_line(line_left); 
    else if (tag > 0 && tag < 4)
      graph_dep.parse_parameter(line_left, tag);	
    else if (tag == 4)
      graph_dep.parse_result(line_left);	
 }

  tracefile.close();

  std::cerr << "=====================END PROFILING STATS====================== " << std::endl;
  std::cerr << std::endl;
  
	graph_dep.output_instid(bench);
  graph_dep.output_memory_trace(bench); 
  graph_dep.output_methodid(bench);
  graph_dep.output_microop(bench);
	graph_dep.output_varid(bench, 1);
	graph_dep.output_varid(bench, 2);
	graph_dep.output_varid(bench, 3);
	graph_dep.output_varid(bench, 4);
  graph_dep.output_parvalue(bench, 1);
  graph_dep.output_parvalue(bench, 2);
  graph_dep.output_parvalue(bench, 3);
  graph_dep.output_parvalue(bench, 4);
  graph_dep.output_partype(bench, 1);
  graph_dep.output_partype(bench, 2);
  graph_dep.output_partype(bench, 3);
  graph_dep.output_partype(bench, 4);
  
  return 0;




}


