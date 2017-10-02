#ifndef __DDDG_H__
#define __DDDG_H__

#include <map>
#include <set>
#include <stack>
#include <string>
#include <zlib.h>
#include <stdlib.h>
#include <sstream>

#include "ExecNode.h"
#include "file_func.h"
#include "Program.h"
#include "opcode_func.h"
#include "SourceManager.h"

#define MEMORY_EDGE -1
#define REGISTER_EDGE 5
#define FUSED_BRANCH_EDGE 6
#define CONTROL_EDGE 11
#define PIPE_EDGE 12

struct reg_edge_t {
  unsigned sink_node;
  int par_id;
};

// data structure used to track dependency
typedef std::unordered_map<std::string, unsigned int> string_to_uint;
typedef std::unordered_map<Addr, unsigned int> uint_to_uint;
typedef std::unordered_multimap<unsigned int, reg_edge_t>
    multi_uint_to_reg_edge;
typedef std::map<unsigned int, std::set<unsigned int>> map_uint_to_set;

class BaseDatapath;

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

    static float ConvertBitsToFloat(uint64_t bits) {
      fp2bits converter;
      converter.bits = bits;
      return converter.fp;
    }

    static double ConvertBitsToDouble(uint64_t bits) {
      fp2bits converter;
      converter.bits = bits;
      return converter.dp;
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


class DDDG {
 public:
  // Indicates that we have reached the end of the trace.
  static const size_t END_OF_TRACE = std::numeric_limits<size_t>::max();

  DDDG(BaseDatapath* _datapath, Program* program, gzFile& _trace_file);
  int num_edges();
  int num_nodes();
  int num_of_register_dependency();
  int num_of_memory_dependency();
  int num_of_control_dependency();
  void output_dddg();
  size_t build_initial_dddg(size_t trace_off, size_t trace_size);
  inline_labelmap_t get_inline_labelmap() { return inline_labelmap; }

 private:
  void parse_instruction_line(std::string line);
  void parse_parameter(std::string line, int param_tag);
  void parse_result(std::string line);
  void parse_forward(std::string line);
  void parse_call_parameter(std::string line, int param_tag);
  void parse_labelmap_line(std::string line);
  std::string parse_function_name(std::string line);
  bool is_function_returned(std::string line, std::string target_function);

  // Enforce RAW/WAW dependencies on this memory access.
  //
  // start_addr: Address of the memory access.
  // size: Number of bytes modified.
  // sink_node: The node corresponding to this memory access.
  void handle_post_write_dependency(Addr start_addr,
                                    size_t size,
                                    unsigned sink_node);
  void insert_control_dependence(unsigned source_node, unsigned dest_node);
  SrcTypes::Variable* get_array_real_var(const std::string& array_name);

  SrcTypes::DynamicFunction curr_dynamic_function;

  uint8_t curr_microop;
  uint8_t prev_microop;
  std::string prev_bblock;
  std::string curr_bblock;
  ExecNode* curr_node;

  SrcTypes::Function* callee_function;
  SrcTypes::DynamicFunction callee_dynamic_function;

  bool last_parameter;
  int num_of_parameters;
  // Used to track the instruction that initialize call function parameters
  int last_call_source;
  // The loop depth of the basic block the current node belongs to.
  unsigned current_loop_depth;
  /* Unique register ID in the caller function. Used to create a mapping between
   * register IDs in caller and callee functions. */
  SrcTypes::DynamicVariable unique_reg_in_caller_func;

  std::string curr_instid;
  std::vector<Addr> parameter_value_per_inst;
  std::vector<unsigned> parameter_size_per_inst;
  std::vector<std::string> parameter_label_per_inst;
  long num_of_instructions;
  long current_node_id;
  int num_of_reg_dep;
  int num_of_mem_dep;
  int num_of_ctrl_dep;
  int last_dma_fence;

  BaseDatapath* datapath;
  Program* program;
  std::string trace_file_name;
  gzFile& trace_file;

  // Register dependency tracking table.
  multi_uint_to_reg_edge register_edge_table;
  // Memory dependence tracking table.
  map_uint_to_set memory_edge_table;
  // Control edge tracking table.
  map_uint_to_set control_edge_table;
  // Maps a label in an inlined function to the original function in which it
  // was written.
  inline_labelmap_t inline_labelmap;

  // keep track of currently executed methods
  std::stack<SrcTypes::DynamicFunction> active_method;
  // manage methods
  std::unordered_map<SrcTypes::DynamicVariable, unsigned> register_last_written;
  uint_to_uint address_last_written;
  // DMA nodes that have been seen since the last DMA fence.
  std::list<unsigned> last_dma_nodes;
  // All nodes seen since the last Ret instruction.
  std::list<unsigned> nodes_since_last_ret;
  ExecNode* last_ret;
  // This points to the SourceManager object inside a BaseDatapath object.
  SrcTypes::SourceManager& srcManager;
};

#endif
