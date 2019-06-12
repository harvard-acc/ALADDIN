#ifndef __DDDG_H__
#define __DDDG_H__

#include <deque>
#include <map>
#include <memory>
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

#define MEMORY_EDGE (-1)
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

// Deserialize the hex string @str to bytes in @byte_buf.
//
// Args:
//   str: A null terminated string, all lowercase, containing only hexadecimal
//        digits, and optionally prefixed by 0x. It must have an even length,
//        UNLESS it is the string "0", which denotes a zero initialized value.
//   size: The size of the buffer to be returned.
//
// Returns:
//   A byte buffer storing the deserialized data. Memory is allocated
//   dynamically, and the caller is responsible for freeing it.
uint8_t* hexStrToBytes(const char* str, unsigned size);

// Serialize the byte buffer into a string.
//
// Args:
//   data: The byte buffer.
//   size: The length of the byte buffer.
//   separate32: If true, then every 32 bits will be separated with an
//     underscore. Defaults to false.
//
// Returns:
//   A string representing the serialized data.
char* bytesToHexStr(uint8_t* data, int size, bool separate32=false);

// This class represents the value of some register or parameter in the trace.
//
// The value can take on different types and formats, depending on the size and
// the format of the value string. This class converts the string into the
// appropriate type and simplifies generic manipulation of the value.
class Value {
 public:
  Value(Value&& other)
      : vector_buf(std::move(other.vector_buf)), data_str(other.data_str),
        data(other.data), type(other.type), size(other.size) {}

  Value(char* value_buf, unsigned _size, bool is_string = false) : size(_size / 8), vector_buf() {
    createValue(value_buf, is_string);
  }

  ~Value() {}

  void createValue(char* value_buf, bool is_string) {
    const std::string value_str(value_buf);
    if (is_string) {
      type = String;
      data_str = value_str;
    } else if (size > 8) {
      type = Vector;
      vector_buf = std::unique_ptr<uint8_t>(hexStrToBytes(value_buf, size));
    } else if (value_str.find('.') != std::string::npos) {
      type = Float;
      if (size == 4) {
        data.fp = (float)strtod(value_str.c_str(), NULL);
      } else if (size == 8) {
        data.dp = strtod(value_str.c_str(), NULL);
      } else {
        assert(false && "Floating point value must be 32-bit or 64-bit!");
      }
    } else if (value_str.substr(0, 2) == "0x") {
      type = Ptr;
      data.ptr = (void*)strtol(value_str.c_str(), NULL, 16);
    } else {
      type = Integer;
      data.integer = (long long)strtol(value_str.c_str(), NULL, 10);
    }
  }

  bool operator==(int value) {
    return data.integer == value;
  }

  bool operator==(float value) {
    return data.fp == value;
  }

  bool operator==(double value) {
    return data.dp == value;
  }

  bool operator==(void* ptr) {
    return data.ptr == ptr;
  }

  // Supported value types.
  enum Type {
    Integer,
    Float,
    Vector,
    Ptr,
    String,
    NumValueTypes,
  };

  // The data accessors we want to expose are scalar, float, string and vector.
  // Even if the type is Ptr, as far as we are concerned, we just need the raw
  // value of the pointer. Only when the data is actually of type Vector do we
  // return a pointer to a character buffer.
  uint64_t getScalar() const { return data.bits; }

  float getFloat() const { return data.fp; }

  // Requesting the vector data buffer pointer will automatically release
  // ownership of the managed pointer.
  uint8_t* getVector() { return vector_buf.release(); }

  const std::string getString() const { return data_str; }

  // Implicit conversion to unsigned integer.
  operator uint64_t() const { return data.bits; }

  Type getType() const { return type; }
  unsigned getSize() const { return size; }

 protected:
  union DataStorage {
    long long integer;
    double dp;
    float fp;
    void* ptr;
    uint64_t bits;
  };

  // Manages the vector data buffer.
  //
  // When we parse a vector value string from the trace, we always convert it
  // into a byte array, but that data doesn't always get used (as part of a
  // MemAccess object).  The unique_ptr is used to handle ownership of that
  // byte array and only free the memory if it was unused.
  std::unique_ptr<uint8_t> vector_buf;

  // String data.
  std::string data_str;

  // Stores the data for all datatypes other than Vector.
  DataStorage data;

  // Data type.
  Type type;
  unsigned size;  // In bytes.
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
  const std::string& get_top_level_function_name() const {
    return top_level_function_name;
  }

 private:
  void parse_instruction_line(const std::string& line);
  void parse_parameter(const std::string& line, int param_tag);
  void parse_result(const std::string& line);
  void parse_forward(const std::string& line);
  void parse_call_parameter(const std::string& line, int param_tag);
  void parse_labelmap_line(const std::string& line);
  void parse_entry_declaration(const std::string& line);
  std::string parse_function_name(const std::string& line);
  bool is_function_returned(const std::string& line, std::string target_function);

  MemAccess* create_mem_access(Value& value);

  // Enforce RAW/WAW dependencies on this memory access.
  //
  // start_addr: Address of the memory access.
  // size: Number of bytes modified.
  // sink_node: The node corresponding to this memory access.
  void handle_post_write_dependency(Addr start_addr,
                                    size_t size,
                                    unsigned sink_node);
  // Enforce memory dependences between ready bit nodes and DMA nodes.
  void handle_ready_bit_dependency(Addr start_addr,
                                   size_t size,
                                   unsigned sink_node);
  void insert_control_dependence(unsigned source_node, unsigned dest_node);
  SrcTypes::Variable* get_array_real_var(const std::string& array_name);
  SrcTypes::Variable* get_array_real_var(SrcTypes::Variable* var);

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
  // The loop depth of the basic block the current node belongs to.
  unsigned current_loop_depth;

  std::string top_level_function_name;
  std::string curr_instid;
  std::vector<Value> parameter_value_per_inst;
  std::vector<unsigned> parameter_size_per_inst;
  std::vector<std::string> parameter_label_per_inst;

  struct FunctionCallerArg {
    FunctionCallerArg() : arg_num(-1), last_node_to_modify(0), is_reg(false) {}
    FunctionCallerArg(int _arg_num,
                      SrcTypes::DynamicVariable _dynvar,
                      unsigned last_node,
                      bool _is_reg)
        : arg_num(_arg_num), dynvar(_dynvar), last_node_to_modify(last_node),
          is_reg(_is_reg) {}
    // Argument number, starting from 0.
    int arg_num;
    // The dynamic variable reference from the caller perspective.
    SrcTypes::DynamicVariable dynvar;
    // The last node to modify this register.
    unsigned last_node_to_modify;
    // Is this variable reference a register or a temporary? If it's a
    // temporary, then last_node_to_modify does not apply.
    bool is_reg;
  };
  std::deque<FunctionCallerArg> func_caller_args;

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

  // keep track of currently executed methods
  std::stack<SrcTypes::DynamicFunction> active_method;
  // manage methods
  std::unordered_map<SrcTypes::DynamicVariable, unsigned> register_last_written;
  uint_to_uint address_last_written;
  uint_to_uint ready_bits_last_changed;
  // DMA nodes that have been seen since the last DMA fence.
  std::list<unsigned> last_dma_nodes;
  // All nodes seen since the last Ret instruction.
  std::list<unsigned> nodes_since_last_ret;
  ExecNode* last_ret;
  // This points to the SourceManager object inside a BaseDatapath object.
  SrcTypes::SourceManager& srcManager;
};

#endif
