#ifndef __NODE_H__
#define __NODE_H__

#include <sstream>
#include <string>

#include <boost/graph/graph_traits.hpp>

#include "opcode_func.h"
#include "boost_typedefs.h"

// Stores all information about a memory access.
struct MemAccess {
  // Address read from the trace.
  long long int vaddr;
  // Size of the memory access in BITS.
  size_t size;
  // If this is not a store, then this value is meaningless.
  // TODO(samxi): Support floating point stores as well.
  long long int value;
};

class ExecNode {

 public:
  ExecNode(unsigned int _node_id, uint8_t _microop)
      : node_id(_node_id), microop(_microop), static_method(""),
        basic_block_id(""), inst_id(""), line_num(-1), execution_cycle(0),
        num_parents(0), isolated(true), inductive(false), dynamic_mem_op(false),
        array_label(""), time_before_execution(0.0), mem_access(nullptr),
        vertex_assigned(false) {}

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
  int get_execution_cycle() { return execution_cycle; }
  int get_num_parents() { return num_parents; }
  Vertex get_vertex() { return vertex; }
  bool is_isolated() { return isolated; }
  bool is_inductive() { return inductive; }
  bool is_dynamic_mem_op() { return dynamic_mem_op; }
  bool has_vertex() { return vertex_assigned; }
  std::string get_array_label() { return array_label; }
  bool has_array_label() { return (array_label.compare("") != 0); }
  MemAccess* get_mem_access() { return mem_access; }
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
  void set_execution_cycle(int cycle) { execution_cycle = cycle; }
  void set_num_parents(int parents) { num_parents = parents; }
  void set_vertex(Vertex vertex) {
    this->vertex = vertex;
    vertex_assigned = true;
  }
  void set_isolated(bool isolated) { this->isolated = isolated; }
  void set_inductive(bool inductive) { this->inductive = inductive; }
  void set_dynamic_mem_op(bool dynamic) { dynamic_mem_op = dynamic; }
  void set_array_label(std::string label) { array_label = label; }
  void set_mem_access(long long int vaddr,
                      size_t size,
                      long long int value = 0) {
    mem_access = new MemAccess;
    mem_access->vaddr = vaddr;
    mem_access->size = size;
    mem_access->value = value;
  }
  void set_time_before_execution(float time) { time_before_execution = time; }

  /* Compound accessors. */
  std::string get_dynamic_method() {
    // TODO: Really inefficient - make something better.
    stringstream oss;
    oss << static_method << "-" << dynamic_invocation;
    return oss.str();
  }
  std::string get_static_node_id() {
    // TODO: Really inefficient - make something better.
    stringstream oss;
    oss << static_method << "-" << dynamic_invocation << "-" << inst_id;
    return oss.str();
  }

  /* Increment/decrement. */
  void decr_num_parents() { num_parents--; }
  void decr_execution_cycle() { execution_cycle--; }

  /* Opcode functions. */
  bool is_associative() {
    if (microop == LLVM_IR_Add)
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
    if (microop == LLVM_IR_Call)
      return true;
    return is_dma_op();
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

  bool is_mul_op() {
    switch (microop) {
      case LLVM_IR_Mul:
      case LLVM_IR_UDiv:
      case LLVM_IR_FMul:
      case LLVM_IR_SDiv:
      case LLVM_IR_FDiv:
      case LLVM_IR_URem:
      case LLVM_IR_SRem:
      case LLVM_IR_FRem:
        return true;
      default:
        return false;
    }
  }

  bool is_add_op() {
    switch (microop) {
      case LLVM_IR_Add:
      case LLVM_IR_FAdd:
      case LLVM_IR_Sub:
      case LLVM_IR_FSub:
        return true;
      default:
        return false;
    }
  }

  float node_latency() {
    if (is_mul_op())
      return MUL_LATENCY;
    if (is_add_op())
      return ADD_LATENCY;
    if (is_memory_op() || microop == LLVM_IR_Ret)
      return MEMOP_LATENCY;
    return 0;
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
  /* Which cycle this node is scheduled for execution. Previously called
   * level. */
  int execution_cycle;
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
  /* Name of the array being accessed if this is a memory operation. */
  std::string array_label;
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
