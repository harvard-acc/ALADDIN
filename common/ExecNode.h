#ifndef __NODE_H__
#define __NODE_H__

#include <cstdint>
#include <sstream>
#include <string>

#include "opcode_func.h"
#include "typedefs.h"

#define BYTE 8

// TODO: Is this correct in 64-bit mode?
//
// Bitmask to ensure that we don't attempt to access above the 32-bit address
// space assuming a 4GB memory. In accelerator simulation with memory system,
// the mem_bus address range is the same as memory size, instead of the 48-bit
// address space in the X86_64 implementation.
#define ADDR_MASK 0xffffffff

// Stores basic information about a typical memory access.
class MemAccess {
  public:
    MemAccess() : vaddr(0), paddr(0), size(0), is_float(false), value(0) {}

    // Address read from the trace.
    Addr vaddr;
    // Physical address (used for caches only).
    Addr paddr;
    // Size of the memory access in bytes.
    size_t size;
    // Is floating-point value or not.
    bool is_float;
    // Hex representation of the value loaded or stored.  For FP values, this is
    // the IEEE-754 representation.
    uint64_t value;
};

class DmaMemAccess : public MemAccess {
  public:
    DmaMemAccess()
        : MemAccess()
        , src_off(0)
        , dst_off(0) {}

    DmaMemAccess(size_t off)
        : MemAccess()
        , src_off(off)
        , dst_off(off) {}

    DmaMemAccess(size_t src_off, size_t dst_off)
        : MemAccess()
        , src_off(src_off)
        , dst_off(dst_off) {}

    /* Additional offset from vaddr/paddr.
     *
     * This is used to work around dependence analysis bugs when dmaLoading
     * from non-base addresses. In the simplest case, src_off and dst_off are
     * equal, but they don't have to be. Double buffering is a common example
     * of where a source offset might be some number N, but the destination
     * offset could be 0, the beginning of the scratchpad.
     */
    size_t src_off;
    size_t dst_off;
};

class ExecNode {

 public:
  ExecNode(unsigned int _node_id, uint8_t _microop)
      : node_id(_node_id), microop(_microop), static_method(""),
        basic_block_id(""), inst_id(""), line_num(-1), start_execution_cycle(0),
        complete_execution_cycle(0), num_parents(0), isolated(true),
        inductive(false), dynamic_mem_op(false), double_precision(false),
        array_label(""), partition_index(0), time_before_execution(0.0),
        mem_access(nullptr), vertex_assigned(false) {}

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
  uint8_t get_microop() { return microop; }
  std::string get_static_method() { return static_method; }
  unsigned int get_dynamic_invocation() { return dynamic_invocation; }
  std::string get_basic_block_id() { return basic_block_id; }
  std::string get_inst_id() { return inst_id; }
  int get_line_num() { return line_num; }
  int get_start_execution_cycle() { return start_execution_cycle; }
  int get_complete_execution_cycle() { return complete_execution_cycle; }
  int get_num_parents() { return num_parents; }
  Vertex get_vertex() { return vertex; }
  bool is_isolated() { return isolated; }
  bool is_inductive() { return inductive; }
  bool is_dynamic_mem_op() { return dynamic_mem_op; }
  bool is_double_precision() { return double_precision; }
  bool has_vertex() { return vertex_assigned; }
  std::string get_array_label() { return array_label; }
  unsigned get_partition_index() { return partition_index; }
  bool has_array_label() { return (array_label.compare("") != 0); }
  MemAccess* get_mem_access() { return mem_access; }
  DmaMemAccess* get_dma_mem_access() { return static_cast<DmaMemAccess*>(mem_access); }
  float get_time_before_execution() { return time_before_execution; }

  /* Setters. */
  void set_microop(uint8_t microop) { this->microop = microop; }
  void set_static_method(std::string method) { static_method = method; }
  void set_dynamic_invocation(unsigned int invocation) {
    dynamic_invocation = invocation;
  }
  void set_basic_block_id(std::string bb_id) { basic_block_id = bb_id; }
  void set_inst_id(std::string id) { inst_id = id; }
  void set_line_num(int line) { line_num = line; }
  void set_start_execution_cycle(int cycle) { start_execution_cycle = cycle; }
  void set_complete_execution_cycle(int cycle) {
    complete_execution_cycle = cycle;
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
  void set_array_label(std::string label) { array_label = label; }
  void set_partition_index(unsigned index) { partition_index = index; }
  void set_mem_access(long long int vaddr,
                      size_t size_in_bytes,
                      bool is_float = false,
                      uint64_t value = 0) {
    mem_access = new MemAccess();
    mem_access->vaddr = vaddr;
    mem_access->size = size_in_bytes;
    mem_access->is_float = is_float;
    mem_access->value = value;
  }
  void set_dma_mem_access(long long int vaddr,
                          size_t src_offset,
                          size_t dst_offset,
                          size_t size_in_bytes,
                          bool is_float = false,
                          uint64_t value = 0) {
    mem_access = new DmaMemAccess(src_offset, dst_offset);
    mem_access->vaddr = vaddr;
    mem_access->size = size_in_bytes;
    mem_access->is_float = is_float;
    mem_access->value = value;
  }
  void set_time_before_execution(float time) { time_before_execution = time; }

  /* Compound accessors. */
  std::string get_dynamic_method() {
    // TODO: Really inefficient - make something better.
    std::stringstream oss;
    oss << static_method << "-" << dynamic_invocation;
    return oss.str();
  }
  std::string get_static_node_id() {
    // TODO: Really inefficient - make something better.
    std::stringstream oss;
    oss << static_method << "-" << dynamic_invocation << "-" << inst_id;
    return oss.str();
  }

  /* Increment/decrement. */
  void decr_num_parents() { num_parents--; }

  /* Opcode functions. */
  bool is_associative() {
    if (is_int_add_op() || is_fp_add_op())
      return true;
    return false;
  }

  bool is_memory_op() {
    if (microop == LLVM_IR_Load || microop == LLVM_IR_Store)
      return true;
    return false;
  }

  bool is_compute_op() {
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

  bool is_store_op() {
    if (microop == LLVM_IR_Store)
      return true;
    return false;
  }

  bool is_load_op() {
    if (microop == LLVM_IR_Load)
      return true;
    return false;
  }

  bool is_shifter_op() {
    if (microop == LLVM_IR_Shl || microop == LLVM_IR_LShr ||
        microop == LLVM_IR_AShr)
      return true;
    return false;
  }

  bool is_bit_op() {
    switch (microop) {
      case LLVM_IR_And:
      case LLVM_IR_Or:
      case LLVM_IR_Xor:
        return true;
      default:
        return false;
    }
  }

  bool is_control_op() {
    if (microop == LLVM_IR_PHI)
      return true;
    return is_branch_op();
  }

  bool is_branch_op() {
    if (microop == LLVM_IR_Br || microop == LLVM_IR_Switch)
      return true;
    return is_call_op();
  }

  bool is_call_op() {
    return (microop == LLVM_IR_Call);
  }

  bool is_index_op() {
    if (microop == LLVM_IR_IndexAdd)
      return true;
    return false;
  }

  bool is_convert_op() {
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

  bool is_dma_load() { return microop == LLVM_IR_DMALoad; }
  bool is_dma_store() { return microop == LLVM_IR_DMAStore; }

  bool is_dma_op() { return is_dma_load() || is_dma_store(); }

  bool is_int_mul_op() {
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

  bool is_int_add_op() {
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
  float fu_node_latency(float cycle_time) {
    if (microop == LLVM_IR_Ret)
      return cycle_time;
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

  bool is_multicycle_op() { return is_fp_op() || is_trig_op(); }

  bool is_fp_op() {
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

  bool is_fp_mul_op() {
    switch (microop) {
      case LLVM_IR_FMul:
      case LLVM_IR_FDiv:  // TODO: Remove once we have a divider model.
      case LLVM_IR_FRem:
        return true;
      default:
        return false;
    }
  }

  bool is_fp_div_op() {
    switch (microop) {
      case LLVM_IR_FDiv:
        return true;
      default:
        return false;
    }
  }

  bool is_fp_add_op() {
    switch (microop) {
      case LLVM_IR_FAdd:
      case LLVM_IR_FSub:
        return true;
      default:
        return false;
    }
  }

  bool is_trig_op() {
    switch (microop) {
      case LLVM_IR_Sine:
      case LLVM_IR_Cosine:
        return true;
      default:
        return false;
    }
  }

  unsigned get_multicycle_latency() {
    if (is_fp_op())
      return fp_node_latency_in_cycles();
    else if (is_trig_op())
      return trig_node_latency_in_cycles();
    return 1;
  }

  unsigned fp_node_latency_in_cycles() {
    if (is_fp_div_op())
      return FP_DIV_LATENCY_IN_CYCLES;
    else if (is_fp_mul_op())
      return FP_MUL_LATENCY_IN_CYCLES;
    else
      return FP_ADD_LATENCY_IN_CYCLES;
  }

  unsigned trig_node_latency_in_cycles() {
    return TRIG_SINE_LATENCY_IN_CYCLES;
  }

 protected:
  /* Unique dynamic node id. */
  unsigned int node_id;
  /* Micro opcode. */
  uint8_t microop;
  /* Name of the function this node belongs to. */
  std::string static_method;
  /* Name of the basic block this node belongs to. */
  std::string basic_block_id;
  /* Unique identifier of the static instruction that generated this node. */
  std::string inst_id;
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
  /* Number of parents of this node. */
  int num_parents;
  /* Corresponding Boost Vertex descriptor. If set_vertex() is never called,
   * then it is default constructor, and vertex_assigned will be false. */
  Vertex vertex;
  /*True if this node is isolated (no parents or children). */
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

 private:
  /* True if the node has been assigned a vertex, false otherwise. */
  bool vertex_assigned;
};

#endif
