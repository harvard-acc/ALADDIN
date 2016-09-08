#include "DDDG.h"
#include "BaseDatapath.h"

DDDG::DDDG(BaseDatapath* _datapath) : datapath(_datapath) {
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

  datapath->setLabelMap(labelmap);
}

// Parse line from the labelmap section.
void DDDG::parse_labelmap_line(std::string line) {
  char label[256];
  int line_number;
  sscanf(line.c_str(), "%s %d", label, &line_number);
  label[255] = '\0';  // Just in case...
  labelmap[line_number] = label;
}

void DDDG::parse_instruction_line(std::string line) {
  char curr_static_function[256];
  char instid[256], bblockid[256];
  int line_num;
  int microop;
  int dyn_inst_count;
  sscanf(line.c_str(),
         "%d,%[^,],%[^,],%[^,],%d,%d\n",
         &line_num,
         curr_static_function,
         bblockid,
         instid,
         &microop,
         &dyn_inst_count);

  num_of_instructions++;
  prev_microop = curr_microop;
  curr_microop = (uint8_t)microop;
  curr_instid = instid;

  curr_node = datapath->insertNode(num_of_instructions, microop);
  curr_node->set_line_num(line_num);
  curr_node->set_inst_id(curr_instid);
  curr_node->set_static_method(curr_static_function);
  datapath->addFunctionName(curr_static_function);

  int func_invocation_count = 0;
  bool curr_func_found = false;

  if (!active_method.empty()) {
    char prev_static_function[256];
    unsigned prev_counts;
    sscanf(active_method.top().c_str(),
           "%[^-]-%u",
           prev_static_function,
           &prev_counts);
    if (strcmp(curr_static_function, prev_static_function) == 0) {
      // calling itself
      if (prev_microop == LLVM_IR_Call &&
          callee_function == curr_static_function) {
        // a new instantiation
        auto func_it = function_counter.find(curr_static_function);
        assert(func_it != function_counter.end());
        func_invocation_count = ++func_it->second;
        std::ostringstream oss;
        oss << curr_static_function << "-" << func_invocation_count;
        curr_dynamic_function = oss.str();
        active_method.push(curr_dynamic_function);
      } else {
        func_invocation_count = prev_counts;
        curr_dynamic_function = active_method.top();
      }
      curr_func_found = true;
    }
    if (microop == LLVM_IR_Ret)
      active_method.pop();
  }
  if (!curr_func_found) {
    auto func_it = function_counter.find(curr_static_function);
    if (func_it == function_counter.end()) {
      func_invocation_count = 0;
      function_counter.insert(
          std::make_pair(curr_static_function, func_invocation_count));
    } else {
      func_invocation_count = ++func_it->second;
    }
    std::ostringstream oss;
    oss << curr_static_function << "-" << func_invocation_count;
    curr_dynamic_function = oss.str();
    active_method.push(curr_dynamic_function);
  }
  if (microop == LLVM_IR_PHI && prev_microop != LLVM_IR_PHI)
    prev_bblock = curr_bblock;
  curr_bblock = bblockid;
  curr_node->set_dynamic_invocation(func_invocation_count);
  last_parameter = 0;
  parameter_value_per_inst.clear();
  parameter_size_per_inst.clear();
  parameter_label_per_inst.clear();
}

void DDDG::parse_parameter(std::string line, int param_tag) {
  int size, is_reg;
  char char_value[256];
  char label[256], prev_bbid[256];
  if (curr_microop == LLVM_IR_PHI) {
    sscanf(line.c_str(),
           "%d,%[^,],%d,%[^,],%[^,],\n",
           &size,
           char_value,
           &is_reg,
           label,
           prev_bbid);
    if (prev_bblock.compare(prev_bbid) != 0) {
      return;
    }
  } else {
    sscanf(line.c_str(),
           "%d,%[^,],%d,%[^,],\n",
           &size,
           char_value,
           &is_reg,
           label);
  }
  bool is_float = false;
  std::string tmp_value(char_value);
  std::size_t found = tmp_value.find('.');
  if (found != std::string::npos)
    is_float = true;
  double value = atof(char_value);
  if (!last_parameter) {
    num_of_parameters = param_tag;
    if (curr_microop == LLVM_IR_Call)
      callee_function = label;
    auto func_it = function_counter.find(callee_function);
    std::ostringstream oss;
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
    if (curr_microop == LLVM_IR_Call) {
      unique_reg_in_caller_func = unique_reg_id;
    }
    // Find the instruction that writes the register
    auto reg_it = register_last_written.find(unique_reg_id);
    if (reg_it != register_last_written.end()) {
      /*Find the last instruction that writes to the register*/
      edge_node_info tmp_edge;
      tmp_edge.sink_node = num_of_instructions;
      tmp_edge.par_id = param_tag;
      register_edge_table.insert(std::make_pair(reg_it->second, tmp_edge));
      num_of_reg_dep++;
      if (curr_microop == LLVM_IR_Call) {
        last_call_source = reg_it->second;
      }
    } else if ((curr_microop == LLVM_IR_Store && param_tag == 2) ||
               (curr_microop == LLVM_IR_Load && param_tag == 1)) {
      /*For the load/store op without a gep instruction before, assuming the
       *load/store op performs a gep which writes to the label register*/
      register_last_written[unique_reg_id] = num_of_instructions;
    }
  }
  if (curr_microop == LLVM_IR_Load || curr_microop == LLVM_IR_Store ||
      curr_microop == LLVM_IR_GetElementPtr || curr_node->is_dma_op()) {
    parameter_value_per_inst.push_back(((Addr)value) & ADDR_MASK);
    parameter_size_per_inst.push_back(size);
    parameter_label_per_inst.push_back(label);
    // last parameter
    if (param_tag == 1 && curr_microop == LLVM_IR_Load) {
      Addr mem_address =
          parameter_value_per_inst.back();
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
          memory_edge_table.insert(std::make_pair(source_inst, tmp_edge));
          num_of_mem_dep++;
        }
      }
      Addr base_address =
          parameter_value_per_inst.back();
      std::string base_label = parameter_label_per_inst.back();
      curr_node->set_array_label(base_label);
      datapath->addArrayBaseAddress(base_label, base_address);
    } else if (param_tag == 2 && curr_microop == LLVM_IR_Store) {
      // 1st arg of store is the value, 2nd arg is the pointer.
      Addr mem_address = parameter_value_per_inst[0];
      auto addr_it = address_last_written.find(mem_address);
      if (addr_it != address_last_written.end())
        addr_it->second = num_of_instructions;
      else
        address_last_written.insert(
            std::make_pair(mem_address, num_of_instructions));

      Addr base_address = parameter_value_per_inst[0];
      std::string base_label = parameter_label_per_inst[0];
      curr_node->set_array_label(base_label);
      datapath->addArrayBaseAddress(base_label, base_address);
    } else if (param_tag == 1 && curr_microop == LLVM_IR_Store) {
      Addr mem_address = parameter_value_per_inst[0];
      unsigned mem_size = parameter_size_per_inst.back() / BYTE_SIZE;
      double store_value = value;
      curr_node->set_mem_access(mem_address, 0, mem_size, is_float, store_value);
    } else if (param_tag == 1 && curr_microop == LLVM_IR_GetElementPtr) {
      Addr base_address = parameter_value_per_inst.back();
      std::string base_label = parameter_label_per_inst.back();
      curr_node->set_array_label(base_label);
      datapath->addArrayBaseAddress(base_label, base_address);
    } else if (param_tag == 1 && curr_node->is_dma_op()) {
      std::string base_label = parameter_label_per_inst.back();
      curr_node->set_array_label(base_label);
    }
  }
}

void DDDG::parse_result(std::string line) {
  int size, is_reg;
  double value;
  char label[256];

  sscanf(line.c_str(), "%d,%lf,%d,%[^,],\n", &size, &value, &is_reg, label);
  if (curr_node->is_fp_op() && (size == 64))
    curr_node->set_double_precision(true);
  assert(is_reg);
  char unique_reg_id[256];
  sprintf(unique_reg_id, "%s-%s", curr_dynamic_function.c_str(), label);
  auto reg_it = register_last_written.find(unique_reg_id);
  if (reg_it != register_last_written.end())
    reg_it->second = num_of_instructions;
  else
    register_last_written[unique_reg_id] = num_of_instructions;

  if (curr_microop == LLVM_IR_Alloca) {
    curr_node->set_array_label(label);
    datapath->addArrayBaseAddress(label, ((Addr)value) & ADDR_MASK);
  } else if (curr_microop == LLVM_IR_Load) {
    Addr mem_address = parameter_value_per_inst.back();
    curr_node->set_mem_access(
        mem_address, 0, size / BYTE_SIZE, value);
  } else if (curr_node->is_dma_op()) {
    Addr mem_address = parameter_value_per_inst[1];
    unsigned mem_offset = (unsigned)parameter_value_per_inst[2];
    unsigned mem_size = (unsigned)parameter_value_per_inst[3];
    curr_node->set_mem_access(mem_address, mem_offset, mem_size);
  }
}

void DDDG::parse_forward(std::string line) {
  int size, is_reg;
  double value;
  char label[256];

  sscanf(line.c_str(), "%d,%lf,%d,%[^,],\n", &size, &value, &is_reg, label);
  assert(is_reg);

  char unique_reg_id[256];
  assert(curr_node->is_call_op() || curr_node->is_dma_op() || curr_node->is_trig_op());
  sprintf(unique_reg_id, "%s-%s", callee_dynamic_function.c_str(), label);
  // Create a mapping between registers in caller and callee functions.
  if (!unique_reg_in_caller_func.empty()) {
    datapath->addCallArgumentMapping(unique_reg_id, unique_reg_in_caller_func);
    unique_reg_in_caller_func.clear();
  }
  auto reg_it = register_last_written.find(unique_reg_id);
  int tmp_written_inst = num_of_instructions;
  if (last_call_source != -1) {
    tmp_written_inst = last_call_source;
  }
  if (reg_it != register_last_written.end())
    reg_it->second = tmp_written_inst;
  else
    register_last_written[unique_reg_id] = tmp_written_inst;
}

std::string DDDG::parse_function_name(std::string line) {
  char curr_static_function[256];
  char instid[256], bblockid[256];
  int line_num;
  int microop;
  int dyn_inst_count;
  sscanf(line.c_str(),
         "%d,%[^,],%[^,],%[^,],%d,%d\n",
         &line_num,
         curr_static_function,
         bblockid,
         instid,
         &microop,
         &dyn_inst_count);
  return curr_static_function;
}

bool DDDG::is_function_returned(std::string line, std::string target_function) {
  char curr_static_function[256];
  char instid[256], bblockid[256];
  int line_num;
  int microop;
  int dyn_inst_count;
  sscanf(line.c_str(),
         "%d,%[^,],%[^,],%[^,],%d,%d\n",
         &line_num,
         curr_static_function,
         bblockid,
         instid,
         &microop,
         &dyn_inst_count);
  if (microop == LLVM_IR_Ret &&
      (!target_function.compare(curr_static_function)))
    return true;
  return false;
}

bool DDDG::build_initial_dddg(gzFile trace_file) {

  std::cout << "-------------------------------" << std::endl;
  std::cout << "      Generating DDDG          " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  char buffer[256];
  std::string first_function;
  bool seen_first_line = false;
  bool first_function_returned = false;
  bool in_labelmap_section = false;
  bool labelmap_parsed_or_not_present = false;
  while (trace_file && !gzeof(trace_file)) {
    if (gzgets(trace_file, buffer, sizeof(buffer)) == NULL) {
      continue;
    }
    std::string wholeline(buffer);

    /* Scan for labelmap section if it has not yet been parsed. */
    if (!labelmap_parsed_or_not_present) {
      if (!in_labelmap_section) {
        if (wholeline.find("%%%% LABEL MAP START %%%%") != std::string::npos) {
          in_labelmap_section = true;
          continue;
        }
      } else {
        if (wholeline.find("%%%% LABEL MAP END %%%%") != std::string::npos) {
          labelmap_parsed_or_not_present = true;
          in_labelmap_section = false;
          continue;
        }
        parse_labelmap_line(wholeline);
      }
    }
    size_t pos_end_tag = wholeline.find(",");

    if (pos_end_tag == std::string::npos) {
      if (first_function_returned)
        break;
      continue;
    }
    // So that we skip that check if we don't have a labelmap.
    labelmap_parsed_or_not_present = true;
    std::string tag = wholeline.substr(0, pos_end_tag);
    std::string line_left = wholeline.substr(pos_end_tag + 1);
    if (tag.compare("0") == 0) {
      if (!seen_first_line) {
        seen_first_line = true;
        first_function = parse_function_name(line_left);
      }
      first_function_returned = is_function_returned(line_left, first_function);
      parse_instruction_line(line_left);
    } else if (tag.compare("r") == 0) {
      parse_result(line_left);
    } else if (tag.compare("f") == 0) {
      parse_forward(line_left);
    } else {
      parse_parameter(line_left, atoi(tag.c_str()));
    }
  }

  output_dddg();

  std::cout << "-------------------------------" << std::endl;
  std::cout << "Num of Nodes: " << datapath->getNumOfNodes() << std::endl;
  std::cout << "Num of Edges: " << datapath->getNumOfEdges() << std::endl;
  std::cout << "Num of Reg Edges: " << num_of_register_dependency()
            << std::endl;
  std::cout << "Num of MEM Edges: " << num_of_memory_dependency() << std::endl;
  std::cout << "-------------------------------" << std::endl;

  return 0;
}
