#ifndef __NODE_H__
#define __NODE_H__

#include <cstdint>
#include <sstream>
#include <string>

#include "MemoryType.h"
#include "opcode_func.h"
#include "typedefs.h"
#include "SourceEntity.h"
#include "SourceManager.h"
#include "DynamicEntity.h"

#define BYTE 8

// TODO: Is this correct in 64-bit mode?
//
// Bitmask to ensure that we don't attempt to access above the 32-bit address
// space assuming a 4GB memory. In accelerator simulation with memory system,
// the mem_bus address range is the same as memory size, instead of the 48-bit
// address space in the X86_64 implementation.
#define ADDR_MASK 0xffffffff

// Basic information about a memory access.
class MemAccess {
  public:
    MemAccess() : vaddr(0), size(0) {}
    MemAccess(Addr _vaddr, size_t _size) : vaddr(_vaddr), size(_size) {}
    virtual ~MemAccess() {}

    // Obtain a pointer to the first byte of data.
    virtual uint8_t* data() = 0;

    // Address read from the trace.
    Addr vaddr;
    // Size of the memory access in bytes.
    size_t size;
};

// For a typical memory access with a value of 64 bits or less.
class ScalarMemAccess : public MemAccess {
  public:
    ScalarMemAccess() : MemAccess(), is_float(false) {}

    void set_value(uint64_t _value) {
      memcpy(&value[0], &_value, 8);
    }

    virtual uint8_t* data() { return &value[0]; }

    // Is this value a floating point value?
    bool is_float;

  protected:
    // Hex representation of the value loaded or stored.  For FP values, this
    // is the IEEE-754 representation.
    uint8_t value[8];
};

// A memory access of greater than 64 bits.
class VectorMemAccess : public MemAccess {
  public:
    VectorMemAccess() : MemAccess(), value(nullptr) {}
    virtual ~VectorMemAccess() {
      if (value)
        delete value;
    }

    void set_value(uint8_t* _value) {
      value = _value;
    }

    virtual uint8_t* data() { return value; }

  protected:
    // A pointer to a dynamically allocated buffer. This mem access object is
    // responsible for cleaning it up.
    uint8_t* value;
};

class HostMemAccess : public MemAccess {
  protected:
   typedef SrcTypes::Variable Variable;

  public:
   HostMemAccess()
       : MemAccess(), memory_type(dma), src_addr(0), src_var(nullptr),
         dst_var(nullptr) {}

   HostMemAccess(MemoryType _memory_type,
                 Addr _dst_addr,
                 Addr _src_addr,
                 size_t _size,
                 Variable* _src_var,
                 Variable* _dst_var)
       : MemAccess(_dst_addr, _size), memory_type(_memory_type),
         src_addr(_src_addr), src_var(_src_var), dst_var(_dst_var) {}

   virtual uint8_t* data() {
     assert(false &&
            "DMA memory accesses do not store the data in the trace itself!");
     return nullptr;
   }

   MemoryType memory_type;
   Addr src_addr;
   Addr& dst_addr = MemAccess::vaddr;

   Variable* src_var;
   Variable* dst_var;
};

class ReadyBitAccess : public MemAccess {
 protected:
  typedef SrcTypes::Variable Variable;

 public:
  ReadyBitAccess() : MemAccess(), value(0) {}
  ReadyBitAccess(Addr addr, size_t size, Variable* _array, uint8_t _value)
      : MemAccess(addr, size), array(_array), value(_value) {}

  virtual uint8_t* data() { return &value; }

  // The array for the ready bits.
  Variable* array;
  // The value the ready bits will be set to: either 0 or 1.
  uint8_t value;
};

class ExecNode {
 protected:
   typedef SrcTypes::src_id_t src_id_t;

 public:
  ExecNode(unsigned int _node_id, uint8_t _microop)
      : node_id(_node_id), microop(_microop), dynamic_invocation(0),
        line_num(-1), start_execution_cycle(-1), complete_execution_cycle(-1),
        dma_scheduling_delay_cycle(0), num_parents(0), isolated(true),
        inductive(false), dynamic_mem_op(false), double_precision(false),
        array_label(""), partition_index(0), time_before_execution(0.0),
        mem_access(nullptr), static_inst(nullptr), static_function(nullptr),
        variable(nullptr), vertex_assigned(false) {}

  ~ExecNode() {
    if (mem_access)
      delete mem_access;
  }
  /* Compare two nodes based only on their node ids. */
  bool operator<(const ExecNode& other) const {
    return (node_id < other.get_node_id());
  }
  bool operator>(const ExecNode& other) const { return (other < *this); }
  bool operator<=(const ExecNode& other) const { return !(other > *this); }
  bool operator>=(const ExecNode& other) const { return !(*this < other); }
  bool operator==(const ExecNode& other) const {
    return (node_id == other.get_node_id());
  }
  bool operator!=(const ExecNode& other) const { return !(*this == other); }

  /* Accessors. */
  unsigned int get_node_id() const { return node_id; }
  uint8_t get_microop() const { return microop; }
  SrcTypes::Function* get_static_function() const { return static_function; }
  SrcTypes::Variable* get_variable() const { return variable; }
  SrcTypes::Instruction* get_static_inst() const { return static_inst; }
  SrcTypes::BasicBlock* get_basic_block() const { return basic_block; }
  unsigned int get_dynamic_invocation() const { return dynamic_invocation; }
  int get_line_num() const { return line_num; }
  bool started() const { return start_execution_cycle != -1; }
  bool completed() const { return complete_execution_cycle != -1; }
  int get_start_execution_cycle() const {
    return !started() ? 0 : start_execution_cycle;
  }
  int get_complete_execution_cycle() const {
    return !completed() ? 0 : complete_execution_cycle;
  }
  int get_dma_scheduling_delay_cycle() const {
    return dma_scheduling_delay_cycle;
  }
  int get_num_parents() const { return num_parents; }
  Vertex get_vertex() const { return vertex; }
  bool is_isolated() const { return isolated; }
  bool is_inductive() const { return inductive; }
  bool is_dynamic_mem_op() const { return dynamic_mem_op; }
  bool is_double_precision() const { return double_precision; }
  bool has_vertex() const { return vertex_assigned; }
  const std::string& get_array_label() const { return array_label; }
  unsigned get_partition_index() const { return partition_index; }
  bool has_array_label() const { return (array_label.compare("") != 0); }
  MemAccess* get_mem_access() const { return mem_access; }
  ScalarMemAccess* get_scalar_mem_access() const {
    return dynamic_cast<ScalarMemAccess*>(mem_access);
  }
  HostMemAccess* get_host_mem_access() const {
    return dynamic_cast<HostMemAccess*>(mem_access);
  }
  VectorMemAccess* get_vector_mem_access() const {
    return dynamic_cast<VectorMemAccess*>(mem_access);
  }
  ReadyBitAccess* get_ready_bit_access() const {
    return dynamic_cast<ReadyBitAccess*>(mem_access);
  }
  unsigned get_loop_depth() const { return loop_depth; }
  const std::string& get_special_math_op() const { return special_math_op; }
  float get_time_before_execution() const { return time_before_execution; }

  /* Setters. */
  void set_microop(uint8_t microop) { this->microop = microop; }
  void set_variable(SrcTypes::Variable* var) { variable = var; }
  void set_static_function(SrcTypes::Function* func) {
    static_function = func;
  }
  void set_basic_block(SrcTypes::BasicBlock* bblock) {
    basic_block = bblock;
  }
  void set_dynamic_invocation(unsigned int invocation) {
    dynamic_invocation = invocation;
  }
  void set_static_inst(SrcTypes::Instruction* inst) { static_inst = inst; }
  void set_line_num(int line) { line_num = line; }
  void set_start_execution_cycle(int cycle) { start_execution_cycle = cycle; }
  void set_complete_execution_cycle(int cycle) {
    complete_execution_cycle = cycle;
  }
  void set_dma_scheduling_delay_cycle(int cycle) {
    dma_scheduling_delay_cycle = cycle;
  }
  void set_num_parents(int parents) { num_parents = parents; }
  void set_vertex(Vertex vertex) {
    this->vertex = vertex;
    vertex_assigned = true;
  }
  void set_isolated(bool isolated) { this->isolated = isolated; }
  void set_inductive(bool inductive) { this->inductive = inductive; }
  void set_dynamic_mem_op(bool dynamic) { dynamic_mem_op = dynamic; }
  void set_double_precision(bool double_precision) {
    this->double_precision = double_precision;
  }
  void set_array_label(const std::string& label) { array_label = label; }
  void set_partition_index(unsigned index) { partition_index = index; }
  void set_mem_access(MemAccess* mem_access) { this->mem_access = mem_access; }
  void set_host_mem_access(HostMemAccess* host_mem_access) {
    mem_access = host_mem_access;
  }
  void set_loop_depth(unsigned depth) { loop_depth = depth; }
  void set_special_math_op(const std::string& name) { special_math_op = name; }
  void set_time_before_execution(float time) { time_before_execution = time; }

  SrcTypes::DynamicFunction get_dynamic_function() const {
    return SrcTypes::DynamicFunction(static_function, dynamic_invocation);
  }

  SrcTypes::DynamicInstruction get_dynamic_instruction() const {
    SrcTypes::DynamicFunction dynfunc = get_dynamic_function();
    return SrcTypes::DynamicInstruction(dynfunc, static_inst);
  }

  SrcTypes::DynamicVariable get_dynamic_variable() const {
    SrcTypes::DynamicFunction dynfunc = get_dynamic_function();
    return SrcTypes::DynamicVariable(dynfunc, variable);
  }

  /* Increment/decrement. */
  void decr_num_parents() { num_parents--; }

  /* Opcode functions. */
  bool is_associative() const {
    if (is_int_add_op() || is_fp_add_op())
      return true;
    return false;
  }

  bool is_intrinsic_op() const { return microop == LLVM_IR_Intrinsic; }

  bool is_gep_op() const {
    return microop == LLVM_IR_GetElementPtr;
  }

  bool is_memory_op() const {
    if (microop == LLVM_IR_Load || microop == LLVM_IR_Store)
      return true;
    return false;
  }

  bool is_compute_op() const {
    switch (microop) {
      case LLVM_IR_Add:
      case LLVM_IR_FAdd:
      case LLVM_IR_Sub:
      case LLVM_IR_FSub:
      case LLVM_IR_Mul:
      case LLVM_IR_FMul:
      case LLVM_IR_UDiv:
      case LLVM_IR_SDiv:
      case LLVM_IR_FDiv:
      case LLVM_IR_URem:
      case LLVM_IR_SRem:
      case LLVM_IR_FRem:
      case LLVM_IR_Shl:
      case LLVM_IR_LShr:
      case LLVM_IR_AShr:
      case LLVM_IR_And:
      case LLVM_IR_Or:
      case LLVM_IR_Xor:
      case LLVM_IR_IndexAdd:
        return true;
      default:
        return false;
    }
  }

  bool is_store_op() const {
    if (microop == LLVM_IR_Store)
      return true;
    return false;
  }

  bool is_load_op() const {
    if (microop == LLVM_IR_Load)
      return true;
    return false;
  }

  bool is_shifter_op() const {
    if (microop == LLVM_IR_Shl || microop == LLVM_IR_LShr ||
        microop == LLVM_IR_AShr)
      return true;
    return false;
  }

  bool is_bit_op() const {
    switch (microop) {
      case LLVM_IR_And:
      case LLVM_IR_Or:
      case LLVM_IR_Xor:
        return true;
      default:
        return false;
    }
  }

  bool is_control_op() const {
    if (microop == LLVM_IR_PHI)
      return true;
    return is_branch_op();
  }

  bool is_branch_op() const {
    if (microop == LLVM_IR_Br || microop == LLVM_IR_Switch)
      return true;
    return is_call_op();
  }

  bool is_call_op() const {
    return (microop == LLVM_IR_Call);
  }

  bool is_ret_op() const {
    return (microop == LLVM_IR_Ret);
  }

  bool is_index_op() const {
    if (microop == LLVM_IR_IndexAdd)
      return true;
    return false;
  }

  bool is_phi_op() const {
      return microop == LLVM_IR_PHI;
  }

  bool is_convert_op() const {
    switch (microop) {
      case LLVM_IR_Trunc:
      case LLVM_IR_ZExt:
      case LLVM_IR_SExt:
      case LLVM_IR_FPToUI:
      case LLVM_IR_FPToSI:
      case LLVM_IR_UIToFP:
      case LLVM_IR_SIToFP:
      case LLVM_IR_FPTrunc:
      case LLVM_IR_FPExt:
      case LLVM_IR_PtrToInt:
      case LLVM_IR_IntToPtr:
      case LLVM_IR_BitCast:
      case LLVM_IR_AddrSpaceCast:
        return true;
      default:
        return false;
    }
  }

  bool is_dma_load() const {
    if (microop == LLVM_IR_DMALoad)
      return true;
    if (microop == LLVM_IR_HostLoad && mem_access)
      return get_host_mem_access()->memory_type == dma;
    return false;
  }
  bool is_dma_store() const {
    if (microop == LLVM_IR_DMAStore)
      return true;
    if (microop == LLVM_IR_HostStore && mem_access)
      return get_host_mem_access()->memory_type == dma;
    return false;
  }
  bool is_dma_fence() const { return microop == LLVM_IR_DMAFence; }
  bool is_set_ready_bits() const { return microop == LLVM_IR_SetReadyBits; }
  bool is_dma_op() const {
    return is_dma_load() || is_dma_store() || is_dma_fence() || is_set_ready_bits();
  }

  bool is_host_load() const {
    return microop == LLVM_IR_DMALoad || microop == LLVM_IR_HostLoad;
  }
  bool is_host_store() const {
    return microop == LLVM_IR_DMAStore || microop == LLVM_IR_HostStore;
  }
  bool is_host_mem_op() const {
    return is_dma_op() || is_host_load() || is_host_store();
  }

  bool is_set_sampling_factor() const {
    return microop == LLVM_IR_SetSamplingFactor;
  }

  bool is_int_mul_op() const {
    switch (microop) {
      case LLVM_IR_Mul:
      case LLVM_IR_UDiv:  // TODO: Divides need special treatment.
      case LLVM_IR_SDiv:
      case LLVM_IR_URem:
      case LLVM_IR_SRem:
        return true;
      default:
        return false;
    }
  }

  bool is_int_add_op() const {
    switch (microop) {
      case LLVM_IR_Add:
      case LLVM_IR_Sub:
        return true;
      default:
        return false;
    }
  }

  /* Node latency for functional units. Should only be called for non-memory
   * operations.
   * Node latencies for memory operations are obtained from memory models
   * (scratchpad or cache) .*/
  float fu_node_latency(float cycle_time) const {
    if (microop == LLVM_IR_Ret)
      return cycle_time;
    if (is_intrinsic_op())
      return intrinsic_op_latency_in_cycles() * cycle_time;
    if (is_fp_op())
      return fp_node_latency_in_cycles() * cycle_time;
    switch ((int)cycle_time) {
      case 6:
        if (is_int_mul_op())
          return MUL_6ns_critical_path_delay;
        else if (is_int_add_op())
          return ADD_6ns_critical_path_delay;
        else if (is_shifter_op())
          return SHIFTER_6ns_critical_path_delay;
        else if (is_bit_op())
          return BIT_6ns_critical_path_delay;
        else
          return 0;
      case 5:
        if (is_int_mul_op())
          return MUL_5ns_critical_path_delay;
        else if (is_int_add_op())
          return ADD_5ns_critical_path_delay;
        else if (is_shifter_op())
          return SHIFTER_5ns_critical_path_delay;
        else if (is_bit_op())
          return BIT_5ns_critical_path_delay;
        else
          return 0;
      case 4:
        if (is_int_mul_op())
          return MUL_4ns_critical_path_delay;
        else if (is_int_add_op())
          return ADD_4ns_critical_path_delay;
        else if (is_shifter_op())
          return SHIFTER_4ns_critical_path_delay;
        else if (is_bit_op())
          return BIT_4ns_critical_path_delay;
        else
          return 0;
      case 3:
        if (is_int_mul_op())
          return MUL_3ns_critical_path_delay;
        else if (is_int_add_op())
          return ADD_3ns_critical_path_delay;
        else if (is_shifter_op())
          return SHIFTER_3ns_critical_path_delay;
        else if (is_bit_op())
          return BIT_3ns_critical_path_delay;
        else
          return 0;
      case 2:
        if (is_int_mul_op())
          return MUL_2ns_critical_path_delay;
        else if (is_int_add_op())
          return ADD_2ns_critical_path_delay;
        else if (is_shifter_op())
          return SHIFTER_2ns_critical_path_delay;
        else if (is_bit_op())
          return BIT_2ns_critical_path_delay;
        else
          return 0;
      case 1:
        if (is_int_mul_op())
          return MUL_1ns_critical_path_delay;
        else if (is_int_add_op())
          return ADD_1ns_critical_path_delay;
        else if (is_shifter_op())
          return SHIFTER_1ns_critical_path_delay;
        else if (is_bit_op())
          return BIT_1ns_critical_path_delay;
        else
          return 0;
      default:
        /* Using 6ns model as the default. */
        if (is_int_mul_op())
          return MUL_6ns_critical_path_delay;
        else if (is_int_add_op())
          return ADD_6ns_critical_path_delay;
        else if (is_shifter_op())
          return SHIFTER_6ns_critical_path_delay;
        else if (is_bit_op())
          return BIT_6ns_critical_path_delay;
        else
          return 0;
    }
  }

  bool is_multicycle_op() const { return is_fp_op() || is_special_math_op(); }

  bool is_fp_op() const {
    switch (microop) {
      case LLVM_IR_FAdd:
      case LLVM_IR_FSub:
      case LLVM_IR_FMul:
      case LLVM_IR_FDiv:
      case LLVM_IR_FRem:
        return true;
      default:
        return false;
    }
  }

  bool is_fp_mul_op() const {
    switch (microop) {
      case LLVM_IR_FMul:
      case LLVM_IR_FDiv:  // TODO: Remove once we have a divider model.
      case LLVM_IR_FRem:
        return true;
      default:
        return false;
    }
  }

  bool is_fp_div_op() const {
    switch (microop) {
      case LLVM_IR_FDiv:
        return true;
      default:
        return false;
    }
  }

  bool is_fp_add_op() const {
    switch (microop) {
      case LLVM_IR_FAdd:
      case LLVM_IR_FSub:
        return true;
      default:
        return false;
    }
  }

  bool is_special_math_op() const {
    switch (microop) {
      case LLVM_IR_SpecialMathOp:
        return true;
      default:
        return false;
    }
  }

  unsigned get_multicycle_latency() const {
    if (is_fp_op())
      return fp_node_latency_in_cycles();
    else if (is_special_math_op()) {
      // TODO: use different latencies for different special math nodes. The
      // name of the special math function is in special_meth_op.
      return special_math_node_latency_in_cycles();
    }
    return 1;
  }

  unsigned fp_node_latency_in_cycles() const {
    if (is_fp_div_op())
      return FP_DIV_LATENCY_IN_CYCLES;
    else if (is_fp_mul_op())
      return FP_MUL_LATENCY_IN_CYCLES;
    else
      return FP_ADD_LATENCY_IN_CYCLES;
  }

  unsigned special_math_node_latency_in_cycles() const {
    return SPECIAL_MATH_NODE_LATENCY_IN_CYCLES;
  }

  unsigned intrinsic_op_latency_in_cycles() const {
    // Intrinsic operations are generally meant to represent functions that
    // would be inefficient to model directly. Since there are so many of them,
    // for now just assume they all take a single cycle. Later, we can refine
    // them if need be based on the operation.
    return 1;
  }

  std::string get_microop_name() const { return opcode_name(microop); }

 protected:
  /* Unique dynamic node id. */
  unsigned int node_id;
  /* Micro opcode. */
  uint8_t microop;
  /* This node came from the ith invocation of the parent function. */
  unsigned int dynamic_invocation;
  /* Corresponding line number from source code. */
  int line_num;
  /* Which cycle this node is scheduled to start execution. */
  int start_execution_cycle;
  /* Which cycle this node is scheduled to complete execution.
   * For non-fp, non-cache-memory ops,
   * start_execution_cycle == complete_execution_cycle */
  int complete_execution_cycle;
  /* This is the delay when the DMA node makes it to the DMA issue queue and is
   * waiting for some cycles before scheduling. The delay is for the DMA setup
   * and cache flush/invalidate overheads. */
  int dma_scheduling_delay_cycle;
  /* Number of parents of this node. */
  int num_parents;
  /* Corresponding Boost Vertex descriptor. If set_vertex() is never called,
   * then it is default constructor, and vertex_assigned will be false. */
  Vertex vertex;
  /* True if this node is isolated (no parents or children).
   *
   * NOTE: This property is NOT safe to check before calling
   * prepareForScheduling(), because it is initialized to true for all nodes.
   * Instead, use boost::degree(node->get_vertex, graph_).
   */
  bool isolated;
  /* True if this node is inductive or has only inductive parents. */
  bool inductive;
  /* True if the node is a dynamic memory operation. */
  bool dynamic_mem_op;
  /* True if the node is a double precision floating point operation. */
  bool double_precision;
  /* Name of the array being accessed if this is a memory operation. */
  std::string array_label;
  /* Index of the partitioned scratchpad being accessed. */
  unsigned partition_index;
  /* Elapsed time before this node executes. Can be a fraction of a cycle.
   * TODO: Maybe refactor this so it's only part of ScratchpadDatapath
   * specifically. Something like a member class that can be extended.
   */
  float time_before_execution;
  /* Stores information about a memory access. If the node is not a memory op,
   * this is NULL.
   */
  MemAccess* mem_access;
  /* Loop depth of the basic block this node belongs to. */
  unsigned loop_depth;
  /* Name of the special math function. */
  std::string special_math_op;

  /* The static instruction that generated this node. */
  SrcTypes::Instruction* static_inst;
  /* The function this node belongs to. */
  SrcTypes::Function* static_function;
  /* The variable this node refers to. */
  SrcTypes::Variable* variable;
  /* The basic block this node refers to. */
  SrcTypes::BasicBlock* basic_block;

 private:
  /* True if the node has been assigned a vertex, false otherwise. */
  bool vertex_assigned;
};

#endif
