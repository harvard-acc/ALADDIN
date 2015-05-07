#include "DDDG.h"
#include "BaseDatapath.h"

gzFile dynamic_func_file;
gzFile instid_file;
gzFile line_num_file;
gzFile memory_trace;
gzFile getElementPtr_trace;

DDDG::DDDG(BaseDatapath* _datapath, std::string _trace_name)
    : datapath(_datapath), trace_name(_trace_name) {
  num_of_reg_dep = 0;
  num_of_mem_dep = 0;
  num_of_instructions = -1;
  last_parameter = 0;
  prev_bblock = "-1";
  curr_bblock = "-1";
}
int DDDG::num_edges() {
  return register_edge_table.size() + memory_edge_table.size();
}
int DDDG::num_nodes() { return num_of_instructions + 1; }
int DDDG::num_of_register_dependency() { return num_of_reg_dep; }
int DDDG::num_of_memory_dependency() { return num_of_mem_dep; }
void DDDG::output_method_call_graph(std::string bench) {
  std::string output_file_name(bench);
  output_file_name += "_method_call_graph";
  write_string_file(
      output_file_name, method_call_graph.size(), method_call_graph);
}
void DDDG::output_dddg() {

  for (auto it = register_edge_table.begin(); it != register_edge_table.end();
       ++it) {
    datapath->addDddgEdge(it->first, it->second.sink_node, it->second.par_id);
  }

  // Memory Dependency
  for (auto it = memory_edge_table.begin(); it != memory_edge_table.end();
       ++it) {
    datapath->addDddgEdge(it->first, it->second.sink_node, it->second.par_id);
  }
}
void DDDG::parse_instruction_line(std::string line) {
  char curr_static_function[256];
  char instid[256], bblockid[256];
  int line_num;
  int microop;
  int count;
  sscanf(line.c_str(),
         "%d,%[^,],%[^,],%[^,],%d,%d\n",
         &line_num,
         curr_static_function,
         bblockid,
         instid,
         &microop,
         &count);

  prev_microop = curr_microop;
  curr_microop = (uint8_t)microop;

  datapath->insertNode(count, microop);

  curr_instid = instid;

  if (!active_method.empty()) {
    char prev_static_function[256];
    unsigned prev_counts;
    sscanf(active_method.top().c_str(),
           "%[^-]-%u",
           prev_static_function,
           &prev_counts);
    if (strcmp(curr_static_function, prev_static_function) != 0) {
      auto func_it = function_counter.find(curr_static_function);
      if (func_it == function_counter.end()) {
        function_counter.insert(make_pair(curr_static_function, 0));
        ostringstream oss;
        oss << curr_static_function << "-0";
        curr_dynamic_function = oss.str();
      } else {
        func_it->second++;
        ostringstream oss;
        oss << curr_static_function << "-" << func_it->second;
        curr_dynamic_function = oss.str();
      }
      active_method.push(curr_dynamic_function);

      // if prev inst is a IRCALL instruction
      if (prev_microop == LLVM_IR_Call) {
        assert(callee_function == curr_static_function);

        ostringstream oss;
        oss << num_of_instructions << "," << prev_static_function << "-"
            << prev_counts << "," << active_method.top();
        method_call_graph.push_back(oss.str());
      }
    } else {  // the same as last method
      // calling it self
      if (prev_microop == LLVM_IR_Call &&
          callee_function == curr_static_function) {
        // a new instantiation
        auto func_it = function_counter.find(curr_static_function);
        assert(func_it != function_counter.end());
        func_it->second++;
        ostringstream oss;
        oss << curr_static_function << "-" << func_it->second;
        curr_dynamic_function = oss.str();
        active_method.push(curr_dynamic_function);
      } else
        curr_dynamic_function = active_method.top();
    }
    if (microop == LLVM_IR_Ret)
      active_method.pop();
  } else {
    auto func_it = function_counter.find(curr_static_function);
    if (func_it != function_counter.end()) {
      func_it->second++;
      ostringstream oss;
      oss << curr_static_function << "-" << func_it->second;
      curr_dynamic_function = oss.str();
    } else {
      function_counter.insert(make_pair(curr_static_function, 0));
      ostringstream oss;
      oss << curr_static_function << "-0";
      curr_dynamic_function = oss.str();
    }
    active_method.push(curr_dynamic_function);
  }
  if (microop == LLVM_IR_PHI && prev_microop != LLVM_IR_PHI)
    prev_bblock = curr_bblock;
  curr_bblock = bblockid;
  gzprintf(dynamic_func_file, "%s\n", curr_dynamic_function.c_str());
  gzprintf(instid_file, "%s\n", curr_instid.c_str());
  gzprintf(line_num_file, "%d\n", line_num);
  num_of_instructions++;
  last_parameter = 0;
  parameter_value_per_inst.clear();
  parameter_size_per_inst.clear();
  parameter_label_per_inst.clear();
}
void DDDG::parse_parameter(std::string line, int param_tag) {
  int size, is_reg;
  double value;
  char label[256], prev_bbid[256];
  if (curr_microop == LLVM_IR_PHI) {
    sscanf(line.c_str(),
           "%d,%lf,%d,%[^,],%[^,],\n",
           &size,
           &value,
           &is_reg,
           label,
           prev_bbid);
    if (prev_bblock.compare(prev_bbid) != 0) {
      return;
    }
  } else {
    sscanf(line.c_str(), "%d,%lf,%d,%[^,],\n", &size, &value, &is_reg, label);
  }
  if (!last_parameter) {
    num_of_parameters = param_tag;
    if (curr_microop == LLVM_IR_Call)
      callee_function = label;
    auto func_it = function_counter.find(callee_function);
    ostringstream oss;
    if (func_it != function_counter.end())
      oss << callee_function << "-" << func_it->second + 1;
    else
      oss << callee_function << "-0";
    callee_dynamic_function = oss.str();
  }
  last_parameter = 1;
  last_call_source = -1;
  if (is_reg) {
    char unique_reg_id[256];
    sprintf(unique_reg_id, "%s-%s", curr_dynamic_function.c_str(), label);
    // Find the instruction that writes the register
    auto reg_it = register_last_written.find(unique_reg_id);
    if (reg_it != register_last_written.end()) {
      edge_node_info tmp_edge;
      tmp_edge.sink_node = num_of_instructions;
      tmp_edge.par_id = param_tag;
      register_edge_table.insert(make_pair(reg_it->second, tmp_edge));
      num_of_reg_dep++;
      if (curr_microop == LLVM_IR_Call)
        last_call_source = reg_it->second;
    }
  }
  if (curr_microop == LLVM_IR_Load || curr_microop == LLVM_IR_Store ||
      curr_microop == LLVM_IR_GetElementPtr || is_dma_op(curr_microop)) {
    parameter_value_per_inst.push_back((long long int)value);
    parameter_size_per_inst.push_back(size);
    parameter_label_per_inst.push_back(label);
    // last parameter
    if (param_tag == 1 && curr_microop == LLVM_IR_Load) {
      long long int mem_address = parameter_value_per_inst.back();
      auto addr_it = address_last_written.find(mem_address);
      if (addr_it != address_last_written.end()) {
        unsigned source_inst = addr_it->second;
        auto same_source_inst = memory_edge_table.equal_range(source_inst);
        bool edge_existed = 0;
        for (auto sink_it = same_source_inst.first;
             sink_it != same_source_inst.second;
             sink_it++) {
          if (sink_it->second.sink_node == num_of_instructions) {
            edge_existed = 1;
            break;
          }
        }
        if (!edge_existed) {
          edge_node_info tmp_edge;
          tmp_edge.sink_node = num_of_instructions;
          // tmp_edge.var_id = "";
          tmp_edge.par_id = -1;
          memory_edge_table.insert(make_pair(source_inst, tmp_edge));
          num_of_mem_dep++;
        }
      }
      long long int base_address = parameter_value_per_inst.back();
      std::string base_label = parameter_label_per_inst.back();
      gzprintf(getElementPtr_trace,
               "%d,%s,%lld\n",
               num_of_instructions,
               base_label.c_str(),
               base_address);
    } else if (param_tag == 2 && curr_microop == LLVM_IR_Store) {
      // 1st arg of store is the value, 2nd arg is the pointer.
      long long int mem_address = parameter_value_per_inst[0];
      auto addr_it = address_last_written.find(mem_address);
      if (addr_it != address_last_written.end())
        addr_it->second = num_of_instructions;
      else
        address_last_written.insert(
            make_pair(mem_address, num_of_instructions));

      long long int base_address = parameter_value_per_inst[0];
      std::string base_label = parameter_label_per_inst[0];
      gzprintf(getElementPtr_trace,
               "%d,%s,%lld\n",
               num_of_instructions,
               base_label.c_str(),
               base_address);
    } else if (param_tag == 1 && curr_microop == LLVM_IR_Store) {
      long long int mem_address = parameter_value_per_inst[0];
      // TODO(samxi): We need to support floating point values as well.
      long long int store_value = parameter_value_per_inst[1];
      unsigned mem_size = parameter_size_per_inst.back();
      gzprintf(memory_trace,
               "%d,%lld,%u,%lld\n",
               num_of_instructions,
               mem_address,
               mem_size,
               store_value);
    } else if (param_tag == 1 && curr_microop == LLVM_IR_GetElementPtr) {
      long long int base_address = parameter_value_per_inst.back();
      std::string base_label = parameter_label_per_inst.back();
      gzprintf(getElementPtr_trace,
               "%d,%s,%lld\n",
               num_of_instructions,
               base_label.c_str(),
               base_address);
    }
  }
}

void DDDG::parse_result(std::string line) {
  int size, is_reg;
  double value;
  char label[256];

  sscanf(line.c_str(), "%d,%lf,%d,%[^,],\n", &size, &value, &is_reg, label);
  assert(is_reg);
  char unique_reg_id[256];
  sprintf(unique_reg_id, "%s-%s", curr_dynamic_function.c_str(), label);
  auto reg_it = register_last_written.find(unique_reg_id);
  if (reg_it != register_last_written.end())
    reg_it->second = num_of_instructions;
  else
    register_last_written[unique_reg_id] = num_of_instructions;

  if (curr_microop == LLVM_IR_Alloca)
    gzprintf(getElementPtr_trace,
             "%d,%s,%lld\n",
             num_of_instructions,
             label,
             (long long int)value);
  else if (curr_microop == LLVM_IR_Load) {
    long long int mem_address = parameter_value_per_inst.back();
    gzprintf(
        memory_trace, "%d,%lld,%u\n", num_of_instructions, mem_address, size);
  } else if (is_dma_op(curr_microop)) {
    long long int mem_address = parameter_value_per_inst[1];
    unsigned mem_size = parameter_value_per_inst[2];
    gzprintf(memory_trace,
             "%d,%lld,%u\n",
             num_of_instructions,
             mem_address,
             mem_size);
  }
}

void DDDG::parse_forward(std::string line) {
  int size, is_reg;
  double value;
  char label[256];

  sscanf(line.c_str(), "%d,%lf,%d,%[^,],\n", &size, &value, &is_reg, label);
  assert(is_reg);

  char unique_reg_id[256];
  assert(is_call_op(curr_microop) || is_dma_op(curr_microop));
  sprintf(unique_reg_id, "%s-%s", callee_dynamic_function.c_str(), label);

  auto reg_it = register_last_written.find(unique_reg_id);
  int tmp_written_inst = num_of_instructions;
  if (last_call_source != -1)
    tmp_written_inst = last_call_source;
  if (reg_it != register_last_written.end())
    reg_it->second = tmp_written_inst;
  else
    register_last_written[unique_reg_id] = tmp_written_inst;
}
bool DDDG::build_initial_dddg() {
  if (!fileExists(trace_name)) {
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << " ERROR: Input Trace Not Found  " << std::endl;
    std::cerr << "-------------------------------" << std::endl;
    return 1;
  } else {
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << "      Generating DDDG          " << std::endl;
    std::cerr << "-------------------------------" << std::endl;
  }

  std::string bench = datapath->getBenchName();
  std::string func_file_name, instid_file_name;
  std::string memory_trace_name, getElementPtr_trace_name;
  std::string resultVar_trace_name, line_num_file_name;
  std::string prevBasicBlock_trace_name;

  func_file_name = bench + "_dynamic_funcid.gz";
  instid_file_name = bench + "_instid.gz";
  line_num_file_name = bench + "_linenum.gz";
  memory_trace_name = bench + "_memaddr.gz";
  getElementPtr_trace_name = bench + "_getElementPtr.gz";
  prevBasicBlock_trace_name = bench + "_prevBasicBlock.gz";

  dynamic_func_file = gzopen(func_file_name.c_str(), "w");
  instid_file = gzopen(instid_file_name.c_str(), "w");
  line_num_file = gzopen(line_num_file_name.c_str(), "w");
  memory_trace = gzopen(memory_trace_name.c_str(), "w");
  getElementPtr_trace = gzopen(getElementPtr_trace_name.c_str(), "w");

  gzFile tracefile_gz = gzopen(trace_name.c_str(), "r");

  char buffer[256];
  while (tracefile_gz && !gzeof(tracefile_gz)) {
    if (gzgets(tracefile_gz, buffer, sizeof(buffer)) == NULL)
      continue;
    std::string wholeline(buffer);
    size_t pos_end_tag = wholeline.find(",");

    if (pos_end_tag == std::string::npos) {
      continue;
    }
    std::string tag = wholeline.substr(0, pos_end_tag);
    std::string line_left = wholeline.substr(pos_end_tag + 1);
    if (tag.compare("0") == 0)
      parse_instruction_line(line_left);
    else if (tag.compare("r") == 0)
      parse_result(line_left);
    else if (tag.compare("f") == 0)
      parse_forward(line_left);
    else
      parse_parameter(line_left, atoi(tag.c_str()));
  }

  gzclose(tracefile_gz);

  gzclose(dynamic_func_file);
  gzclose(instid_file);
  gzclose(line_num_file);
  gzclose(memory_trace);
  gzclose(getElementPtr_trace);

  output_dddg();

  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "Num of Nodes: " << datapath->getNumOfNodes() << std::endl;
  std::cerr << "Num of Edges: " << datapath->getNumOfEdges() << std::endl;
  std::cerr << "Num of Reg Edges: " << num_of_register_dependency()
            << std::endl;
  std::cerr << "Num of MEM Edges: " << num_of_memory_dependency() << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  return 0;
}
