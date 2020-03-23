#include <cstring>
#include <stdio.h>
#include <sys/stat.h>

#include <boost/tokenizer.hpp>

#include "BaseDatapath.h"
#include "DDDG.h"
#include "DynamicEntity.h"
#include "ExecNode.h"
#include "ProgressTracker.h"
#include "SourceManager.h"
#include "user_config.h"

using namespace SrcTypes;

static const char kHexTable[16] = {
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

static uint8_t nibbleVal(char nib) {
  for (uint8_t i = 0; i < 16; i++) {
    if (nib == kHexTable[i])
      return i;
  }
  assert("Invalid character!\n");
  return 0;
}

uint8_t* hexStrToBytes(const char* str, unsigned size) {
  unsigned len = strlen(str);
  if (len == 1 && str[0] == '0') {
    // Zero initialized value.
    uint8_t* byte_buf = new uint8_t[size];
    memset((void*)byte_buf, 0, size);
    return byte_buf;
  }
  assert(len % 2 == 0 && "String must be an even length!");
  unsigned start = 0;
  unsigned byte_buf_len = len/2;
  // Ignore the starting 0x.
  if (strncmp(str, "0x", 2) == 0) {
    start += 2;
    byte_buf_len--;
  }
  assert(byte_buf_len == size &&
         "Length of string does not match buffer size!");
  uint8_t* byte_buf = new uint8_t[byte_buf_len];
  for (unsigned i = start; i < len; i += 2) {
    uint8_t val = (nibbleVal(str[i]) << 4) + nibbleVal(str[i + 1]);
    byte_buf[(i - start) / 2] = val;
  }
  return byte_buf;
}

char* bytesToHexStr(uint8_t* data, int size, bool separate32) {
  unsigned str_len = size * 2 + 3;
  if (separate32)
    str_len += size / 4;

  unsigned sep_count = 0;
  const unsigned sep_threshold = separate32 ? 4 : size + 1;

  char* str_buf = new char[str_len];
  char* buf_ptr = str_buf;
  sprintf(buf_ptr, "0x");
  buf_ptr += 2;

  for (int i = 0; i < size; i++) {
    buf_ptr += sprintf(buf_ptr, "%02x", data[i]);
    sep_count++;
    if (sep_count == sep_threshold && i != size - 1) {
      buf_ptr += sprintf(buf_ptr, "_");
      sep_count = 0;
    }
  }
  *buf_ptr = 0;

  return str_buf;
}

// TODO: Eventual goal is to remove datapath as an argument entirely and rely
// only on Program.
DDDG::DDDG(BaseDatapath* _datapath, Program* _program, gzFile& _trace_file)
    : datapath(_datapath), program(_program), trace_file(_trace_file),
      srcManager(_datapath->get_source_manager()) {
  num_of_reg_dep = 0;
  num_of_mem_dep = 0;
  num_of_ctrl_dep = 0;
  num_of_instructions = -1;
  current_node_id = -1;
  last_parameter = 0;
  last_dma_fence = -1;
  prev_bblock = "-1";
  curr_bblock = "-1";
  current_loop_depth = 0;
  callee_function = nullptr;
  last_ret = nullptr;
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
    program->addEdge(it->first, it->second.sink_node, it->second.par_id);
  }

  for (auto source_it = memory_edge_table.begin();
       source_it != memory_edge_table.end();
       ++source_it) {
    unsigned source = source_it->first;
    std::set<unsigned>& sink_list = source_it->second;
    for (unsigned sink_node : sink_list)
      program->addEdge(source, sink_node, MEMORY_EDGE);
  }

  for (auto source_it = control_edge_table.begin();
       source_it != control_edge_table.end();
       ++source_it) {
    unsigned source = source_it->first;
    std::set<unsigned>& sink_list = source_it->second;
    for (unsigned sink_node : sink_list)
      program->addEdge(source, sink_node, CONTROL_EDGE);
  }
}

void DDDG::handle_ready_bit_dependency(Addr start_addr,
                                       size_t size,
                                       unsigned sink_node) {
  std::set<unsigned> ready_bit_nodes;
  // Find the last node that updated the full/empty state for this address.
  for (Addr addr = start_addr; addr < start_addr + size; addr++) {
    auto it = ready_bits_last_changed.find(addr);
    if (it != ready_bits_last_changed.end()) {
      ready_bit_nodes.insert(it->second);
      // This dependency only lasts until the next DMA operation that changes
      // or depends on this ready bit.
      ready_bits_last_changed.erase(it);
    }
  }
  // Add the dependence.
  for (unsigned source_inst : ready_bit_nodes) {
    if (memory_edge_table.find(source_inst) == memory_edge_table.end())
      memory_edge_table[source_inst] = std::set<unsigned>();
    std::set<unsigned>& sink_list = memory_edge_table[source_inst];
    auto result = sink_list.insert(sink_node);
    if (result.second)
      num_of_mem_dep++;
  }
}

void DDDG::handle_post_write_dependency(Addr start_addr,
                                        size_t size,
                                        unsigned sink_node) {
  Addr addr = start_addr;
  bool is_ready_mode = datapath->isReadyMode();
  ExecNode* sink = program->nodes.at(sink_node);
  while (addr < start_addr + size) {
    // Get the last node to write to this address.
    auto addr_it = address_last_written.find(addr);
    if (addr_it != address_last_written.end()) {
      unsigned source_inst = addr_it->second;

      // If sink_node is a load, source_inst is a DMA load, and ready mode is
      // enabled, we can skip adding this dependency, because ready mode
      // replaces the DDDG for handling RAW dependencies where the READ is from
      // an ordinary load.
      ExecNode* source = program->nodes.at(source_inst);
      if (!(sink->is_load_op() && source->is_dma_load() && is_ready_mode)) {
        if (memory_edge_table.find(source_inst) == memory_edge_table.end())
          memory_edge_table[source_inst] = std::set<unsigned>();
        std::set<unsigned>& sink_list = memory_edge_table[source_inst];

        // No need to check if the node already exists - if it does, an
        // insertion will not happen, and insertion would require searching for
        // the place to place the new entry anyways.
        auto result = sink_list.insert(sink_node);
        if (result.second)
          num_of_mem_dep++;
      }
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
// Return a pointer to that Variable.
Variable* DDDG::get_array_real_var(const std::string& array_name) {
  Variable* var = srcManager.get<Variable>(array_name);
  return get_array_real_var(var);
}

// Same as the above function, but takes a Variable* pointer directly.
Variable* DDDG::get_array_real_var(Variable* var) {
  DynamicVariable dyn_var(curr_dynamic_function, var);
  DynamicVariable real_dyn_var = program->call_arg_map.lookup(dyn_var);
  Variable* real_var = real_dyn_var.get_variable();
  return real_var;
}

MemAccess* DDDG::create_mem_access(Value& value) {
  if (value.getType() == Value::Vector) {
    VectorMemAccess* mem_access = new VectorMemAccess();
    mem_access->set_value(value.getVector());
    mem_access->size = value.getSize();
    return mem_access;
  } else {
    ScalarMemAccess* mem_access = new ScalarMemAccess();
    mem_access->set_value(value.getScalar());
    mem_access->is_float = value.getType() == Value::Float;
    mem_access->size = value.getSize();
    return mem_access;
  }
}

// Parse line from the labelmap section.
void DDDG::parse_labelmap_line(const std::string& line) {
  char label_name[256], function_name[256], callers[256];
  int line_number;
  int num_matches = sscanf(line.c_str(),
                           "%[^/]/%s %d inline %[^\n]",
                           function_name,
                           label_name,
                           &line_number,
                           callers);
  label_name[255] = '\0';  // Just in case...
  function_name[255] = '\0';
  Function* function = srcManager.insert<Function>(function_name);
  Label* label = srcManager.insert<Label>(label_name);
  UniqueLabel unique_label(function, label, line_number);
  program->labelmap.insert(std::make_pair(line_number, unique_label));
  if (num_matches == 4) {
    boost::char_separator<char> sep(" ");
    std::string temp(callers);
    boost::tokenizer<boost::char_separator<char>> tok(temp, sep);
    for (auto it = tok.begin(); it != tok.end(); ++it) {
      std::string caller_name = *it;
      Function* caller_func = srcManager.insert<Function>(caller_name);
      UniqueLabel inlined_label(caller_func, label, line_number);
      program->labelmap.insert(std::make_pair(line_number, inlined_label));
      // Add the inlined labels to another map so that we can associate any
      // unrolling/pipelining directives declared on the original labels with
      // them.
      program->inline_labelmap[inlined_label] = unique_label;
    }
  }
}

void DDDG::parse_instruction_line(const std::string& line) {
  char curr_static_function[256];
  char instid[256], bblockid[256], bblockname[256];
  int line_num;
  int microop;
  sscanf(line.c_str(),
         "%d,%[^,],%[^,],%[^,],%d,%lu\n",
         &line_num,
         curr_static_function,
         bblockid,
         instid,
         &microop,
         &current_node_id);

  num_of_instructions++;
  prev_microop = curr_microop;
  curr_microop = (uint8_t)microop;
  curr_instid = instid;

  // Update the current loop depth.
  sscanf(bblockid, "%[^:]:%u", bblockname, &current_loop_depth);
  // If the loop depth is greater than 1000 within this function, we've
  // probably done something wrong.
  assert(current_loop_depth < 1000 &&
         "Loop depth is much higher than expected!");

  Function* curr_function =
      srcManager.insert<Function>(curr_static_function);
  Instruction* curr_inst = srcManager.insert<Instruction>(curr_instid);
  BasicBlock* basicblock = srcManager.insert<BasicBlock>(bblockname);
  curr_node = program->insertNode(current_node_id, microop);
  curr_node->set_line_num(line_num);
  curr_node->set_static_inst(curr_inst);
  curr_node->set_static_function(curr_function);
  curr_node->set_basic_block(basicblock);
  curr_node->set_loop_depth(current_loop_depth);
  datapath->addFunctionName(curr_static_function);
  if (top_level_function_name.empty())
    top_level_function_name = std::string(curr_static_function);

  int func_invocation_count = 0;
  bool curr_func_found = false;

  // Enforce dependences on function call boundaries.
  // Another function cannot be called until all previous nodes in the current
  // function have finished, and a function must execute all nodes before nodes
  // in the parent function can execute. The only exceptions are DMA nodes.
  if (curr_node->is_ret_op() || curr_node->is_call_op()) {
    for (auto node_id : nodes_since_last_ret)
      insert_control_dependence(node_id, current_node_id);
    nodes_since_last_ret.clear();
    if (last_ret && last_ret != curr_node)
      insert_control_dependence(last_ret->get_node_id(), current_node_id);
    last_ret = curr_node;
  } else if (!curr_node->is_dma_op()) {
    nodes_since_last_ret.push_back(current_node_id);
  }

  if (!active_method.empty()) {
    Function* prev_function = active_method.top().get_function();
    unsigned prev_counts = prev_function->get_invocations();
    if (curr_function == prev_function) {
      // calling itself
      if (prev_microop == LLVM_IR_Call && callee_function == curr_function) {
        curr_function->increment_invocations();
        func_invocation_count = curr_function->get_invocations();
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
    curr_function->increment_invocations();
    func_invocation_count = curr_function->get_invocations();
    active_method.push(DynamicFunction(curr_function));
    curr_dynamic_function = active_method.top();
  }
  if (microop == LLVM_IR_PHI && prev_microop != LLVM_IR_PHI)
    prev_bblock = curr_bblock;
  if (microop == LLVM_IR_DMAFence) {
    last_dma_fence = current_node_id;
    for (unsigned node_id : last_dma_nodes) {
      insert_control_dependence(node_id, last_dma_fence);
    }
    last_dma_nodes.clear();
  } else if (microop == LLVM_IR_DMALoad || microop == LLVM_IR_DMAStore ||
             microop == LLVM_IR_SetReadyBits) {
    if (last_dma_fence != -1)
      insert_control_dependence(last_dma_fence, current_node_id);
    last_dma_nodes.push_back(current_node_id);
  }
  curr_bblock = bblockid;
  curr_node->set_dynamic_invocation(func_invocation_count);
  last_parameter = false;
  parameter_value_per_inst.clear();
  parameter_size_per_inst.clear();
  parameter_label_per_inst.clear();
  func_caller_args.clear();
}

void DDDG::parse_parameter(const std::string& line, int param_tag) {
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
  Value value(
      char_value,
      size,
      // Only the first argument of the setSamplingFactor function is a string.
      param_tag == 1 && curr_microop == LLVM_IR_SetSamplingFactor);
  if (curr_microop == LLVM_IR_EntryDecl) {
    if (value.getType() == Value::Ptr)
      datapath->addEntryArrayDecl(std::string(label), value);
    return;
  }
  if (!last_parameter) {
    num_of_parameters = param_tag;
    if (curr_microop == LLVM_IR_Call)
      callee_function = srcManager.insert<Function>(label);
    if (callee_function) {
      callee_dynamic_function = DynamicFunction(
          callee_function, callee_function->get_invocations() + 1);
    }
    if (curr_microop == LLVM_IR_SpecialMathOp) {
      curr_node->set_special_math_op(label);
    }
  }
  last_parameter = true;
  if (is_reg) {
    Variable* variable = srcManager.insert<Variable>(label);
    DynamicVariable unique_reg_ref(curr_dynamic_function, variable);
    auto reg_it = register_last_written.find(unique_reg_ref);
    bool found_reg_entry = reg_it != register_last_written.end();
    if (curr_microop == LLVM_IR_Call && param_tag != num_of_parameters) {
      // The first parameter on a call function block is the name of the
      // function itself, not an argument to the function.
      //
      // The first element in the pair is the register reference for this call
      // argument.  The second element in the pair is the id of the last node
      // to write to this register, if such a node exists.
      unsigned last_node_to_modify =
          found_reg_entry ? reg_it->second : current_node_id;
      func_caller_args.push_back(
          FunctionCallerArg(param_tag, unique_reg_ref, last_node_to_modify, true));
    }
    // Find the instruction that writes the register
    if (found_reg_entry) {
      /*Find the last instruction that writes to the register*/
      reg_edge_t tmp_edge = { (unsigned)current_node_id, param_tag };
      register_edge_table.insert(std::make_pair(reg_it->second, tmp_edge));
      num_of_reg_dep++;
    } else if ((curr_microop == LLVM_IR_Store && param_tag == 2) ||
               (curr_microop == LLVM_IR_Load && param_tag == 1)) {
      /*For the load/store op without a gep instruction before, assuming the
       *load/store op performs a gep which writes to the label register*/
      register_last_written[unique_reg_ref] = current_node_id;
    }
  } else {
    if (curr_microop == LLVM_IR_Call && param_tag != num_of_parameters) {
      // If a function is called with a literal value instead of a variable,
      // there isn't a name associated with the argument. Therefore, we'll have
      // to pick a placeholder variable to use for this temporary. The empty
      // string will work fine.
      Variable* temporary = srcManager.insert<Variable>("");
      DynamicVariable unique_ref(curr_dynamic_function, temporary);
      func_caller_args.push_back(
          FunctionCallerArg(param_tag, unique_ref, 0, false));
    }
  }
  if (curr_microop == LLVM_IR_Load || curr_microop == LLVM_IR_Store ||
      curr_microop == LLVM_IR_GetElementPtr || curr_node->is_host_mem_op() ||
      curr_node->is_set_sampling_factor()) {
    // NOTE: We are moving value into the parameter value list, so the value
    // object is now no longer valid and cannot be used directly!
    parameter_value_per_inst.push_back(std::move(value));
    parameter_size_per_inst.push_back(size);
    parameter_label_per_inst.push_back(label);
    // last parameter
    if (param_tag == 1 && curr_microop == LLVM_IR_Load) {
      // The label is the name of the register that holds the address.
      const std::string& reg_name = parameter_label_per_inst.back();
      Variable* var = srcManager.get<Variable>(reg_name);
      curr_node->set_variable(var);
      curr_node->set_array_label(reg_name);
    } else if (param_tag == 1 && curr_microop == LLVM_IR_Store) {
      // 1st arg of store is the address, and the 2nd arg is the value, but the
      // 2nd arg is parsed first.
      Addr mem_address = parameter_value_per_inst[0];
      MemAccess* mem_access =
          create_mem_access(parameter_value_per_inst.back());
      mem_access->vaddr = mem_address;
      curr_node->set_mem_access(mem_access);
    } else if (param_tag == 2 && curr_microop == LLVM_IR_Store) {
      Addr mem_address = parameter_value_per_inst[0];
      unsigned mem_size = parameter_size_per_inst.back() / BYTE;

      auto addr_it = address_last_written.find(mem_address);
      if (addr_it != address_last_written.end()) {
        // Found a WAW dependency.
        handle_post_write_dependency(
            mem_address, mem_size, current_node_id);
        // Now we can overwrite the last written node id.
        addr_it->second = current_node_id;
      } else {
        address_last_written.insert(
            std::make_pair(mem_address, current_node_id));
      }

      // The label is the name of the register that holds the address.
      const std::string& reg_name = parameter_label_per_inst[0];
      Variable* var = srcManager.get<Variable>(reg_name);
      curr_node->set_variable(var);
      curr_node->set_array_label(reg_name);
    } else if (param_tag == 1 && curr_microop == LLVM_IR_GetElementPtr) {
      Addr base_address = parameter_value_per_inst.back();
      const std::string& base_label = parameter_label_per_inst.back();
      // The variable id should be set to the current perceived array name,
      // since that's how dependencies are locally enforced.
      Variable* var = srcManager.get<Variable>(base_label);
      curr_node->set_variable(var);
      // Only GEPs have an array label we can use to update the base address.
      Variable* real_array = get_array_real_var(base_label);
      const std::string& real_name = real_array->get_name();
      curr_node->set_array_label(real_name);
      datapath->addArrayBaseAddress(real_name, base_address);
    } else if (param_tag == 1 && curr_node->is_host_mem_op()) {
      // Data dependencies are handled in parse_result(), because we need all
      // the arguments to dmaLoad in order to do this.
    } else if (curr_node->is_set_sampling_factor()) {
      // The information of the sampled loops will be parsed in
      // parse_result().
    }
  }
}

void DDDG::parse_result(const std::string& line) {
  int size, is_reg;
  char char_value[256];
  char label[256];

  sscanf(line.c_str(), "%d,%[^,],%d,%[^,],\n", &size, char_value, &is_reg, label);
  Value value(char_value, size);
  std::string label_str(label);

  if (curr_node->is_fp_op() && (size == 64))
    curr_node->set_double_precision(true);
  assert(is_reg);
  Variable* var = srcManager.insert<Variable>(label_str);
  DynamicVariable unique_reg_ref(curr_dynamic_function, var);
  register_last_written[unique_reg_ref] = current_node_id;

  if (curr_microop == LLVM_IR_Alloca) {
    curr_node->set_variable(srcManager.get<Variable>(label_str));
    curr_node->set_array_label(label_str);
    datapath->addArrayBaseAddress(label_str, value.getScalar());
  } else if (curr_microop == LLVM_IR_Load) {
    Addr mem_address = parameter_value_per_inst.back();
    size_t mem_size = size / BYTE;
    MemAccess* mem_access = create_mem_access(value);
    mem_access->vaddr = mem_address;
    handle_post_write_dependency(mem_address, mem_size, current_node_id);
    curr_node->set_mem_access(mem_access);
  } else if (curr_node->is_host_load() || curr_node->is_host_store()) {
    HostMemAccess* mem_access;
    Addr src_addr = 0, dst_addr = 0;
    size_t size = 0;
    Variable *src_var = nullptr, *dst_var = nullptr;
    int dma_version = 0;
    if (value == 3) {
      // Version 3 is the only version with a non-zero return value.
      dma_version = 3;
    } else if (num_of_parameters == 5 && value == 0) {
      // Version 2 has 4 arguments + the function name = 5 parameters, and
      // return value 0.
      dma_version = 2;
    } else {
      // Otherwise, it must be version 1.
      dma_version = 1;
    }
    if (dma_version == 1) {
      Addr base_addr = parameter_value_per_inst[1];
      size_t offset = (size_t) parameter_value_per_inst[2];
      size = (size_t) parameter_value_per_inst[3];
      src_addr = base_addr + offset;
      dst_addr = src_addr;
      src_var = srcManager.get<Variable>(parameter_label_per_inst[1]);
      dst_var = src_var;
    } else if (dma_version == 2) {
      Addr base_addr = parameter_value_per_inst[1];
      size_t src_offset = (size_t) parameter_value_per_inst[2];
      size_t dst_offset = (size_t) parameter_value_per_inst[3];
      size = (size_t) parameter_value_per_inst[4];
      src_addr = base_addr + src_offset;
      dst_addr = base_addr + dst_offset;
      src_var = srcManager.get<Variable>(parameter_label_per_inst[1]);
      dst_var = src_var;
    } else if (dma_version == 3) {
      dst_addr = parameter_value_per_inst[1];
      src_addr = parameter_value_per_inst[2];
      size = (size_t) parameter_value_per_inst[3];
      dst_var = srcManager.get<Variable>(parameter_label_per_inst[1]);
      src_var = srcManager.get<Variable>(parameter_label_per_inst[2]);
    } else {
      assert(false && "Invalid DMA interface version!");
    }

    src_var = get_array_real_var(src_var);
    dst_var = get_array_real_var(dst_var);
    // We set the default host memory access type to DMA here, because we don't
    // know yet the original host array label, which may be set to use a
    // different memory type by the user via the setArrayMemoryType API. We
    // handle that in the DMA base address init pass, where the original array
    // label of host node will be found.
    mem_access =
        new HostMemAccess(dma, dst_addr, src_addr, size, src_var, dst_var);
    curr_node->set_host_mem_access(mem_access);

    if (curr_microop == LLVM_IR_DMALoad || curr_microop == LLVM_IR_HostLoad) {
      /* For dmaLoad (which is a STORE from the accelerator's perspective),
       * enforce RAW and WAW dependencies on subsequent nodes.
       *
       * NOTE: If we're using full/empty bits, then we want loads to issue as
       * soon as their data is available, rather than depending on the DMA load
       * node to finish. However, stores must still be ordered with respect to
       * DMA loads. Therefore, we must still update the address_last_written
       * state and skip adding edges to load nodes later.
       */
      for (Addr addr = dst_addr; addr < dst_addr + size; addr += 1) {
        // TODO: Storing an entry for every byte in this range is very
        // inefficient...
        address_last_written[addr] = current_node_id;
      }
      // DMALoads also change ready bits.
      if (datapath->isReadyMode())
        handle_ready_bit_dependency(dst_addr, size, current_node_id);
    } else {
      // For dmaStore (which is actually a LOAD from the accelerator's
      // perspective), enforce RAW dependencies on this node.
      handle_post_write_dependency(src_addr, size, current_node_id);
    }
  } else if (curr_node->is_set_ready_bits()) {
    Addr start_addr = parameter_value_per_inst[1];
    size_t size = parameter_value_per_inst[2];
    unsigned value = parameter_value_per_inst[3];
    for (Addr addr = start_addr; addr < start_addr + size; addr++) {
      // Changing ready bits can be viewed as a store; this means we can use
      // the existing dependence infrastructure to handle the dependence
      // between ready bit changes and loads and stores.
      address_last_written[addr] = current_node_id;
      ready_bits_last_changed[addr] = current_node_id;
    }
    Variable* var = srcManager.get<Variable>(parameter_label_per_inst[1]);
    var = get_array_real_var(var);
    ReadyBitAccess* access =
        new ReadyBitAccess(start_addr, size, var, value);
    curr_node->set_mem_access(access);
  } else if (curr_node->is_set_sampling_factor()) {
    const std::string& loop_name = parameter_value_per_inst[1].getString();
    float factor = parameter_value_per_inst[2].getFloat();
    Label* label = srcManager.insert<Label>(loop_name);
    DynamicLabel dyn_label(curr_node->get_dynamic_function(), label);
    program->loop_info.addSamplingFactor(dyn_label, factor);
  }
}

void DDDG::parse_forward(const std::string& line) {
  int size, is_reg;
  char label_buf[256];

  // DMA and trig operations are not actually treated as called functions by
  // Aladdin, so there is no need to add any register name mappings.
  if (curr_node->is_host_mem_op() || curr_node->is_special_math_op() ||
      curr_node->is_set_sampling_factor())
    return;

  sscanf(line.c_str(), "%d,%*[^,],%d,%[^,],\n", &size, &is_reg, label_buf);
  assert(is_reg);

  assert(curr_node->is_call_op());
  Variable* var = srcManager.insert<Variable>(label_buf);
  DynamicVariable unique_reg_ref(callee_dynamic_function, var);
  // Create a mapping between registers in caller and callee functions.
  FunctionCallerArg arg;
  if (!func_caller_args.empty()) {
    arg = func_caller_args.front();
    if (arg.is_reg)
      program->call_arg_map.add(unique_reg_ref, arg.dynvar);
    func_caller_args.pop_front();
  } else {
    std::cerr
        << "[WARNING]: Expected more forwarded arguments for call to function "
        << active_method.top().get_function()->get_name() << " at node "
        << curr_node->get_node_id() << "!\n";
  }
  if (arg.is_reg) {
    if (arg.dynvar) {
      register_last_written[unique_reg_ref] = arg.last_node_to_modify;
    } else {
      register_last_written[unique_reg_ref] = current_node_id;
    }
  }
}

void DDDG::parse_entry_declaration(const std::string& line) {
  curr_microop = LLVM_IR_EntryDecl;
  char curr_static_function[256];
  num_of_parameters = 0;
  sscanf(line.c_str(), "%[^,],%d\n", curr_static_function, &num_of_parameters);
  top_level_function_name = std::string(curr_static_function);
}

std::string DDDG::parse_function_name(const std::string& line) {
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

bool DDDG::is_function_returned(const std::string& line, std::string target_function) {
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
    } else if (tag.compare("entry") == 0) {
      parse_entry_declaration(line_left);
    } else {
      parse_parameter(line_left, atoi(tag.c_str()));
    }
  }

  if (seen_first_line) {
    output_dddg();

    std::cout << "-------------------------------" << std::endl;
    std::cout << "Num of Nodes: " << program->getNumNodes() << std::endl;
    std::cout << "Num of Edges: " << program->getNumEdges() << std::endl;
    std::cout << "Num of Reg Edges: " << num_of_register_dependency()
              << std::endl;
    std::cout << "Num of MEM Edges: " << num_of_memory_dependency()
              << std::endl;
    std::cout << "Num of Control Edges: " << num_of_control_dependency()
              << std::endl;
    std::cout << "-------------------------------" << std::endl;
    return static_cast<size_t>(current_trace_off);
  } else {
    // The trace (or whatever was left) was empty.
    std::cout << "-------------------------------" << std::endl;
    std::cout << "Reached end of trace." << std::endl;
    std::cout << "-------------------------------" << std::endl;
    return END_OF_TRACE;
  }
}
