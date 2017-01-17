#include <stdio.h>
#include <sys/stat.h>

#include "BaseDatapath.h"
#include "DDDG.h"
#include "DynamicEntity.h"
#include "ExecNode.h"
#include "ProgressTracker.h"
#include "SourceManager.h"

class FP2BitsConverter {
  public:
    /* Convert a float, double, or integer into its hex representation.
     *
     * By hex representation, we mean a 32-bit or 64-bit integer type whose
     * value is the IEEE-754 format that represents either a float or double,
     * respectively.
     *
     * Args:
     *  value: The value to convert as a double precision float.
     *  size: The size of the floating point value (floats and doubles have
     *     different hex representations for the same number).
     *  is_float: If false, then instead of a conversion, a cast is performed
     *     on value from double to uint64_t.
     *
     *  Returns:
     *    A uint64_t value that represents the given floating point value. If
     *    the value was a double precision number, then all 64-bits are used.
     *    If the value was a float, then the first 32-bits are zero. If
     *    is_float was false, then this value is just the floating point value
     *    casted to this type.
     */
    static uint64_t Convert(double value, size_t size, bool is_float) {
      if (!is_float)
        return (uint64_t) value;

      if (size == sizeof(float))
        return toFloat(value);
      else if (size == sizeof(double))
        return toDouble(value);
      else
        assert(false && "Size was not either sizeof(float) or sizeof(double)!");
    }

  private:
    union fp2bits {
      double dp;
      // Relying on the compiler to insert the appropriate zero padding for the
      // smaller sized float object in this union.
      float fp;
      uint64_t bits;
    };

    static uint64_t toFloat(double value) {
      fp2bits converter;
      converter.bits = 0;
      converter.fp = value;
      return converter.bits;
    }
    static uint64_t toDouble(double value) {
      fp2bits converter;
      converter.bits = 0;
      converter.dp = value;
      return converter.bits;
    }
};

DDDG::DDDG(BaseDatapath* _datapath, gzFile& _trace_file)
    : datapath(_datapath), trace_file(_trace_file),
      srcManager(_datapath->get_source_manager()) {
  num_of_reg_dep = 0;
  num_of_mem_dep = 0;
  num_of_ctrl_dep = 0;
  num_of_instructions = -1;
  last_parameter = 0;
  last_dma_fence = -1;
  prev_bblock = "-1";
  curr_bblock = "-1";
}

int DDDG::num_edges() {
  return num_of_reg_dep + num_of_mem_dep + num_of_ctrl_dep;
}

int DDDG::num_nodes() { return num_of_instructions + 1; }
int DDDG::num_of_register_dependency() { return num_of_reg_dep; }
int DDDG::num_of_memory_dependency() { return num_of_mem_dep; }
int DDDG::num_of_control_dependency() { return num_of_ctrl_dep; }

void DDDG::output_dddg() {
  for (auto it = register_edge_table.begin(); it != register_edge_table.end();
       ++it) {
    datapath->addDddgEdge(it->first, it->second.sink_node, it->second.par_id);
  }

  for (auto source_it = memory_edge_table.begin();
       source_it != memory_edge_table.end();
       ++source_it) {
    unsigned source = source_it->first;
    std::set<unsigned>& sink_list = source_it->second;
    for (unsigned sink_node : sink_list)
      datapath->addDddgEdge(source, sink_node, MEMORY_EDGE);
  }

  for (auto source_it = control_edge_table.begin();
       source_it != control_edge_table.end();
       ++source_it) {
    unsigned source = source_it->first;
    std::set<unsigned>& sink_list = source_it->second;
    for (unsigned sink_node : sink_list)
      datapath->addDddgEdge(source, sink_node, CONTROL_EDGE);
  }
}

void DDDG::handle_post_write_dependency(Addr start_addr,
                                        size_t size,
                                        unsigned sink_node) {
  Addr addr = start_addr;
  while (addr < start_addr + size) {
    // Get the last node to write to this address.
    auto addr_it = address_last_written.find(addr);
    if (addr_it != address_last_written.end()) {
      unsigned source_inst = addr_it->second;

      if (memory_edge_table.find(source_inst) == memory_edge_table.end())
        memory_edge_table[source_inst] = std::set<unsigned>();
      std::set<unsigned>& sink_list = memory_edge_table[source_inst];

      // No need to check if the node already exists - if it does, an insertion
      // will not happen, and insertion would require searching for the place
      // to place the new entry anyways.
      auto result = sink_list.insert(sink_node);
      if (result.second)
        num_of_mem_dep++;
    }
    addr++;
  }
}

void DDDG::insert_control_dependence(unsigned source_node, unsigned dest_node) {
  if (control_edge_table.find(source_node) == control_edge_table.end())
    control_edge_table[source_node] = std::set<unsigned>();
  std::set<unsigned>& dest_nodes = control_edge_table[source_node];
  auto result = dest_nodes.insert(dest_node);
  if (result.second)
    num_of_ctrl_dep++;
}

// Find the original array corresponding to this array in the current function.
//
// The array_name argument may not actually be the real name of the array as it
// was originally declared, so we have to backtrace dynamic variable references
// until we find the original one.
//
// Return a reference to that Variable.
const Variable& DDDG::get_array_real_var(const std::string& array_name) {
  Variable var = srcManager.get<Variable>(array_name);
  DynamicVariable dyn_var(curr_dynamic_function, var);
  DynamicVariable real_dyn_var = datapath->getCallerRegID(dyn_var);
  const Variable& real_var = srcManager.get<Variable>(real_dyn_var.get_variable_id());
  return real_var;
}

// Parse line from the labelmap section.
void DDDG::parse_labelmap_line(std::string line) {
  char label_name[256], function_name[256];
  int line_number;
  sscanf(line.c_str(), "%[^/]/%s %d", function_name, label_name, &line_number);
  label_name[255] = '\0';  // Just in case...
  function_name[255] = '\0';
  Function& function = srcManager.insert<Function>(function_name);
  Label& label = srcManager.insert<Label>(label_name);
  UniqueLabel unique_label(function, label);
  labelmap.insert(std::make_pair(line_number, unique_label));
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

  Function& curr_function =
      srcManager.insert<Function>(curr_static_function);
  Instruction& curr_inst = srcManager.insert<Instruction>(curr_instid);
  curr_node = datapath->insertNode(num_of_instructions, microop);
  curr_node->set_line_num(line_num);
  curr_node->set_static_inst_id(curr_inst.get_id());
  curr_node->set_static_function_id(curr_function.get_id());
  datapath->addFunctionName(curr_static_function);

  int func_invocation_count = 0;
  bool curr_func_found = false;

  if (!active_method.empty()) {
    const Function& prev_function =
        srcManager.get<Function>(active_method.top().get_function_id());
    unsigned prev_counts = prev_function.get_invocations();
    if (curr_function == prev_function) {
      // calling itself
      if (prev_microop == LLVM_IR_Call && callee_function == curr_function) {
        curr_function.increment_invocations();
        func_invocation_count = curr_function.get_invocations();
        active_method.push(DynamicFunction(curr_function));
        curr_dynamic_function = active_method.top();
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
    // This would only be true on a call.
    curr_function.increment_invocations();
    func_invocation_count = curr_function.get_invocations();
    active_method.push(DynamicFunction(curr_function));
    curr_dynamic_function = active_method.top();
  }
  if (microop == LLVM_IR_PHI && prev_microop != LLVM_IR_PHI)
    prev_bblock = curr_bblock;
  if (microop == LLVM_IR_DMAFence) {
    last_dma_fence = num_of_instructions;
    for (unsigned node_id : last_dma_nodes) {
      insert_control_dependence(node_id, last_dma_fence);
    }
    last_dma_nodes.clear();
  } else if (microop == LLVM_IR_DMALoad || microop == LLVM_IR_DMAStore) {
    if (last_dma_fence != -1)
      insert_control_dependence(last_dma_fence, num_of_instructions);
    last_dma_nodes.push_back(num_of_instructions);
  }
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
  double value = strtod(char_value, NULL);
  if (!last_parameter) {
    num_of_parameters = param_tag;
    if (curr_microop == LLVM_IR_Call)
      callee_function = srcManager.insert<Function>(label);
    callee_dynamic_function =
        DynamicFunction(callee_function, callee_function.get_invocations() + 1);
  }
  last_parameter = 1;
  last_call_source = -1;
  if (is_reg) {
    Variable& variable = srcManager.insert<Variable>(label);
    DynamicVariable unique_reg_ref(curr_dynamic_function, variable);
    if (curr_microop == LLVM_IR_Call) {
      unique_reg_in_caller_func = unique_reg_ref;
    }
    // Find the instruction that writes the register
    auto reg_it = register_last_written.find(unique_reg_ref);
    if (reg_it != register_last_written.end()) {
      /*Find the last instruction that writes to the register*/
      reg_edge_t tmp_edge = { (unsigned)num_of_instructions, param_tag };
      register_edge_table.insert(std::make_pair(reg_it->second, tmp_edge));
      num_of_reg_dep++;
      if (curr_microop == LLVM_IR_Call) {
        last_call_source = reg_it->second;
      }
    } else if ((curr_microop == LLVM_IR_Store && param_tag == 2) ||
               (curr_microop == LLVM_IR_Load && param_tag == 1)) {
      /*For the load/store op without a gep instruction before, assuming the
       *load/store op performs a gep which writes to the label register*/
      register_last_written[unique_reg_ref] = num_of_instructions;
    }
  }
  if (curr_microop == LLVM_IR_Load || curr_microop == LLVM_IR_Store ||
      curr_microop == LLVM_IR_GetElementPtr || curr_node->is_dma_op()) {
    parameter_value_per_inst.push_back(((Addr)value) & ADDR_MASK);
    parameter_size_per_inst.push_back(size);
    parameter_label_per_inst.push_back(label);
    // last parameter
    if (param_tag == 1 && curr_microop == LLVM_IR_Load) {
      // The label is the name of the register that holds the address.
      const std::string& reg_name = parameter_label_per_inst.back();
      src_id_t var_id = srcManager.get_id<Variable>(reg_name);
      curr_node->set_variable_id(var_id);
      curr_node->set_array_label(reg_name);
    } else if (param_tag == 1 && curr_microop == LLVM_IR_Store) {
      // 1st arg of store is the address, and the 2nd arg is the value, but the
      // 2nd arg is parsed first.
      Addr mem_address = parameter_value_per_inst[0];
      unsigned mem_size = parameter_size_per_inst.back() / BYTE;
      uint64_t bits = FP2BitsConverter::Convert(value, mem_size, is_float);
      curr_node->set_mem_access(mem_address, mem_size, is_float, bits);
    } else if (param_tag == 2 && curr_microop == LLVM_IR_Store) {
      Addr mem_address = parameter_value_per_inst[0];
      unsigned mem_size = parameter_size_per_inst.back() / BYTE;

      auto addr_it = address_last_written.find(mem_address);
      if (addr_it != address_last_written.end()) {
        // Check if the last node to write was a DMA load. If so, we must obey
        // this memory ordering, because DMA loads are variable-latency
        // operations.
        int last_node_to_write = addr_it->second;
        if (datapath->getNodeFromNodeId(last_node_to_write)->is_dma_load())
          handle_post_write_dependency(
              mem_address, mem_size, num_of_instructions);
        // Now we can overwrite the last written node id.
        addr_it->second = num_of_instructions;
      } else {
        address_last_written.insert(
            std::make_pair(mem_address, num_of_instructions));
      }

      // The label is the name of the register that holds the address.
      const std::string& reg_name = parameter_label_per_inst[0];
      src_id_t var_id = srcManager.get_id<Variable>(reg_name);
      curr_node->set_variable_id(var_id);
      curr_node->set_array_label(reg_name);
    } else if (param_tag == 1 && curr_microop == LLVM_IR_GetElementPtr) {
      Addr base_address = parameter_value_per_inst.back();
      const std::string& base_label = parameter_label_per_inst.back();
      // The variable id should be set to the current perceived array name,
      // since that's how dependencies are locally enforced.
      src_id_t var_id = srcManager.get_id<Variable>(base_label);
      curr_node->set_variable_id(var_id);
      // Only GEPs have an array label we can use to update the base address.
      const Variable& real_array = get_array_real_var(base_label);
      const std::string& real_name = real_array.get_name();
      curr_node->set_array_label(real_name);
      datapath->addArrayBaseAddress(real_name, base_address);
    } else if (param_tag == 1 && curr_node->is_dma_op()) {
      // Data dependencies are handled in parse_result(), because we need all
      // the arguments to dmaLoad in order to do this.
    }
  }
}

void DDDG::parse_result(std::string line) {
  int size, is_reg;
  char char_value[256];
  char label[256];

  sscanf(line.c_str(), "%d,%[^,],%d,%[^,],\n", &size, char_value, &is_reg, label);
  bool is_float = false;
  std::string tmp_value(char_value);
  std::size_t found = tmp_value.find('.');
  if (found != std::string::npos)
    is_float = true;
  double value = strtod(char_value, NULL);
  std::string label_str(label);

  if (curr_node->is_fp_op() && (size == 64))
    curr_node->set_double_precision(true);
  assert(is_reg);
  Variable& var = srcManager.insert<Variable>(label_str);
  DynamicVariable unique_reg_ref(curr_dynamic_function, var);
  auto reg_it = register_last_written.find(unique_reg_ref);
  if (reg_it != register_last_written.end())
    reg_it->second = num_of_instructions;
  else
    register_last_written[unique_reg_ref] = num_of_instructions;

  if (curr_microop == LLVM_IR_Alloca) {
    curr_node->set_variable_id(srcManager.get_id<Variable>(label_str));
    curr_node->set_array_label(label_str);
    datapath->addArrayBaseAddress(label_str, ((Addr)value) & ADDR_MASK);
  } else if (curr_microop == LLVM_IR_Load) {
    Addr mem_address = parameter_value_per_inst.back();
    size_t mem_size = size / BYTE;
    handle_post_write_dependency(mem_address, mem_size, num_of_instructions);
    uint64_t bits = FP2BitsConverter::Convert(value, mem_size, is_float);
    curr_node->set_mem_access(mem_address, mem_size, is_float, bits);
  } else if (curr_node->is_dma_op()) {
    Addr base_addr;
    size_t src_off, dst_off, size;
    // Determine DMA interface version.
    if (parameter_value_per_inst.size() == 4) {
      // v1 (src offset = dst offset).
      base_addr = parameter_value_per_inst[1];
      src_off = (size_t) parameter_value_per_inst[2];
      dst_off = src_off;
      size = (size_t) parameter_value_per_inst[3];
    } else if (parameter_value_per_inst.size() == 5) {
      // v2 (src offset is separate from dst offset).
      base_addr = parameter_value_per_inst[1];
      src_off = (size_t) parameter_value_per_inst[2];
      dst_off = (size_t) parameter_value_per_inst[3];
      size = (size_t) parameter_value_per_inst[4];
    }
    curr_node->set_dma_mem_access(base_addr, src_off, dst_off, size);
    if (curr_microop == LLVM_IR_DMALoad) {
      /* If we're using full/empty bits, then we want loads and stores to
       * issue as soon as their data is available. This means that for nearly
       * all of the loads, the DMA load node would not have completed, so we
       * can't add these memory dependencies.
       */
      if (!datapath->isReadyMode()) {
        // For dmaLoad (which is a STORE from the accelerator's perspective),
        // enforce RAW and WAW dependencies on subsequent nodes.
        Addr start_addr = base_addr + dst_off;
        for (Addr addr = start_addr; addr < start_addr + size; addr += 1) {
          // TODO: Storing an entry for every byte in this range is very inefficient...
          auto addr_it = address_last_written.find(addr);
          if (addr_it != address_last_written.end())
            addr_it->second = num_of_instructions;
          else
            address_last_written.insert(
                std::make_pair(addr, num_of_instructions));
        }
      }
    } else {
      // For dmaStore (which is actually a LOAD from the accelerator's
      // perspective), enforce RAW dependencies on this node.
      Addr start_addr = base_addr + src_off;
      handle_post_write_dependency(start_addr, size, num_of_instructions);
    }
  }
}

void DDDG::parse_forward(std::string line) {
  int size, is_reg;
  double value;
  char label[256];

  sscanf(line.c_str(), "%d,%lf,%d,%[^,],\n", &size, &value, &is_reg, label);
  assert(is_reg);

  assert(curr_node->is_call_op() || curr_node->is_dma_op() || curr_node->is_trig_op());
  Variable& var = srcManager.insert<Variable>(label);
  DynamicVariable unique_reg_ref(callee_dynamic_function, var);
  // Create a mapping between registers in caller and callee functions.
  if (unique_reg_in_caller_func != InvalidDynamicVariable) {
    datapath->addCallArgumentMapping(unique_reg_ref, unique_reg_in_caller_func);
    unique_reg_in_caller_func = InvalidDynamicVariable;
  }
  auto reg_it = register_last_written.find(unique_reg_ref);
  int tmp_written_inst = num_of_instructions;
  if (last_call_source != -1) {
    tmp_written_inst = last_call_source;
  }
  if (reg_it != register_last_written.end())
    reg_it->second = tmp_written_inst;
  else
    register_last_written[unique_reg_ref] = tmp_written_inst;
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

size_t DDDG::build_initial_dddg(size_t trace_off, size_t trace_size) {

  std::cout << "-------------------------------" << std::endl;
  std::cout << "      Generating DDDG          " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  long current_trace_off = trace_off;
  // Bigger traces would benefit from having a finer progress report.
  float increment = trace_size > 5e8 ? 0.01 : 0.05;
  // The total progress is the amount of the trace parsed.
  ProgressTracker trace_progress(
      "dddg_parse_progress.out", &current_trace_off, trace_size, increment);
  trace_progress.add_stat("nodes", &num_of_instructions);
  trace_progress.add_stat("bytes", &current_trace_off);

  char buffer[256];
  std::string first_function;
  bool seen_first_line = false;
  bool first_function_returned = false;
  bool in_labelmap_section = false;
  bool labelmap_parsed_or_not_present = false;
  trace_progress.start_epoch();
  while (trace_file && !gzeof(trace_file)) {
    if (gzgets(trace_file, buffer, sizeof(buffer)) == NULL) {
      continue;
    }
    current_trace_off = gzoffset(trace_file);
    if (trace_progress.at_epoch_end()) {
      trace_progress.start_new_epoch();
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
  std::cout << "Num of Control Edges: " << num_of_control_dependency() << std::endl;
  std::cout << "-------------------------------" << std::endl;

  return static_cast<size_t>(current_trace_off);
}
