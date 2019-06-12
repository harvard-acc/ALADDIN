#ifndef _DYNAMIC_ENTITY_H_
#define _DYNAMIC_ENTITY_H_

/* Classes for dynamic objects.
 *
 * Dynamic objects are those that only exist in the context of an execution.
 * They refer to a SourceEntity or a pairing of SourceEntity objects at a
 * particular moment in time. Because these are dynamic, they should not be stored
 * in the SourceManager, so they do not inherit from SourceEntity.
 *
 * To test for invalidity of Dynamic objects, simply check for boolean trueness:
 *
 *   DynamicVariable var = ...;
 *   if (var) {
 *     // var is valid.
 *   }
 */

#include "SourceEntity.h"
#include "SourceManager.h"

namespace SrcTypes {

// Represents a specific invocation of a function.
//
// This pairs a Function with an iteration. For efficiency, this stores
// the function id instead of a reference to the Function object itself.
class DynamicFunction {
 public:
  DynamicFunction() : function(nullptr), iteration(-1) {}
  DynamicFunction(Function* f)
      : function(f), iteration(f->get_invocations()) {}
  DynamicFunction(Function* f, unsigned it)
      : function(f), iteration(it) {}

  explicit operator bool() const {
    return (function && iteration != (unsigned)-1);
  }

  bool operator==(const DynamicFunction& other) const {
    return (*other.function == *function && other.iteration == iteration);
  }

  DynamicFunction& operator=(const DynamicFunction& other) {
    function = other.function;
    iteration = other.iteration;
    return *this;
  }

  // Convert to a string representation (which is the old DDDG format).
  std::string str() const {
    std::stringstream oss;
    oss << function->get_name() << "-" << iteration;
    return oss.str();
  }

  Function* get_function() const { return function; }
  unsigned get_iteration() const { return iteration; }

 private:
  Function* function;
  unsigned iteration;
};

// Represents a dynamic reference to a variable.
//
// This pairs a Variable with a DynamicFunction (in which it is referred to).
class DynamicVariable {
 public:
  DynamicVariable() : dyn_func(), variable(nullptr) {}

  DynamicVariable(DynamicFunction df, Variable* v)
      : dyn_func(df), variable(v) {}

  explicit operator bool() const {
    return (dyn_func && variable);
  }

  bool operator==(const DynamicVariable& other) const {
    return (dyn_func == other.dyn_func && *other.variable == *variable);
  }

  bool operator!=(const DynamicVariable& other) const {
    return !(this->operator==(other));
  }

  DynamicVariable& operator=(const DynamicVariable& other) {
    dyn_func = other.dyn_func;
    variable = other.variable;
    return *this;
  }

  const DynamicFunction* get_dynamic_function() const { return &dyn_func; }
  Variable* get_variable() const { return variable; }

  // Convert to a string representation (which is the old DDDG format).
  std::string str() const {
    Function* func = dyn_func.get_function();
    std::stringstream oss;
    oss << func->get_name() << "-" << dyn_func.get_iteration() << "-"
        << variable->get_name();
    return oss.str();
  }

 private:
  DynamicFunction dyn_func;
  Variable* variable;
};

// Represents an instruction in a particular invocation of a function.
class DynamicInstruction {
 public:
  DynamicInstruction() : dyn_func(), instruction(nullptr) {}
  DynamicInstruction(DynamicFunction df, Instruction* i)
      : dyn_func(df), instruction(i) {}

  explicit operator bool() const {
    return (dyn_func && instruction);
  }

  bool operator==(const DynamicInstruction& other) const {
    return (dyn_func == other.dyn_func && *instruction == *other.instruction);
  }

  bool operator!=(const DynamicInstruction& other) const {
    return !(this->operator==(other));
  }

  DynamicInstruction& operator=(const DynamicInstruction& other) {
    dyn_func = other.dyn_func;
    instruction = other.instruction;
    return *this;
  }

  std::string str() const {
    std::stringstream oss;
    oss << dyn_func.str() << "-" << instruction->get_name();
    return oss.str();
  }

  const DynamicFunction* get_dynamic_function() const { return &dyn_func; }
  Instruction* get_instruction() const { return instruction; }

 private:
  DynamicFunction dyn_func;
  Instruction* instruction;
};

// Represents a label in a particular invocation of a function.
class DynamicLabel {
 public:
  DynamicLabel() : dyn_func(), label(nullptr) {}
  DynamicLabel(DynamicFunction df, Label* l, int ln = 0)
      : dyn_func(df), label(l), line_number(ln) {}

  explicit operator bool() const { return (dyn_func && label); }

  bool operator==(const DynamicLabel& other) const {
    return (dyn_func == other.dyn_func && *label == *other.label &&
            line_number == other.line_number);
  }

  bool operator!=(const DynamicLabel& other) const {
    return !(this->operator==(other));
  }

  DynamicLabel& operator=(const DynamicLabel& other) {
    dyn_func = other.dyn_func;
    label = other.label;
    line_number = other.line_number;
    return *this;
  }

  std::string str() const {
    std::stringstream oss;
    oss << dyn_func.str() << "-" << label->get_name() << "-" << line_number;
    return oss.str();
  }

  void set_line_number(int ln) { line_number = ln; }

  const DynamicFunction* get_dynamic_function() const { return &dyn_func; }
  Label* get_label() const { return label; }
  int get_line_number() const { return line_number; }

 private:
  DynamicFunction dyn_func;
  Label* label;
  int line_number;
};

};  // namespace SrcTypes

// Hashing functions to enable these classes to be stored as keys in hash maps.
namespace std {
template <> struct hash<SrcTypes::DynamicFunction> {
  size_t operator()(const SrcTypes::DynamicFunction& f) const {
    // Compute individual hash values for two data members and combine them
    // using XOR and bit shifting
    if (f) {
      return (f.get_function()->get_id() ^
              (std::hash<unsigned>()(f.get_iteration()) << 1) >> 1);
    }
    return SrcTypes::InvalidId;
  }
};

template <> struct hash<SrcTypes::DynamicVariable> {
  size_t operator()(const SrcTypes::DynamicVariable& v) const {
    if (v) {
      size_t func_hash = std::hash<SrcTypes::DynamicFunction>()(*v.get_dynamic_function());
      return (func_hash ^ (v.get_variable()->get_id() << 2) >> 2);
    }
    return SrcTypes::InvalidId;
  }
};

template <> struct hash<SrcTypes::DynamicInstruction> {
  size_t operator()(const SrcTypes::DynamicInstruction& di) const {
    if (di) {
      const SrcTypes::DynamicFunction* dynfunc = di.get_dynamic_function();
      SrcTypes::src_id_t inst_id = di.get_instruction()
                                       ? di.get_instruction()->get_id()
                                       : SrcTypes::InvalidId;
      return (std::hash<SrcTypes::DynamicFunction>()(*dynfunc) ^
              (inst_id << 2) >> 2);
    }
    return SrcTypes::InvalidId;
  }
};

template <> struct hash<SrcTypes::DynamicLabel> {
  size_t operator()(const SrcTypes::DynamicLabel& dl) const {
    if (dl) {
      const SrcTypes::DynamicFunction* dynfunc = dl.get_dynamic_function();
      SrcTypes::src_id_t label_id =
          dl.get_label() ? dl.get_label()->get_id() : SrcTypes::InvalidId;
      return (std::hash<SrcTypes::DynamicFunction>()(*dynfunc) ^
              (label_id << 2) >> 2);
    }
    return SrcTypes::InvalidId;
  }
};

};


#endif
