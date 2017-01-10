#ifndef _DYNAMIC_ENTITY_H_
#define _DYNAMIC_ENTITY_H_

/* Classes for dynamic objects.
 *
 * Dynamic objects are those that only exist in the context of an execution.
 * They refer to a SourceEntity or a pairing of SourceEntity objects at a
 * particular moment in time. Because these are dynamic, they should not be stored
 * in the SourceManager, so they do not inherit from SourceEntity.
 */

#include "SourceEntity.h"
#include "SourceManager.h"

// Represents a specific invocation of a function.
//
// This pairs a Function with an iteration. For efficiency, this stores
// the function id instead of a reference to the Function object itself.
class DynamicFunction {
 public:
  DynamicFunction() : func_id(-1), iteration(-1) {}
  DynamicFunction(Function& f)
      : func_id(f.get_id()), iteration(f.get_invocations()) {}
  DynamicFunction(Function& f, unsigned it)
      : func_id(f.get_id()), iteration(it) {}
  DynamicFunction(src_id_t _func_id, unsigned it)
      : func_id(_func_id), iteration(it) {}

  bool operator==(const DynamicFunction& other) const {
    return (other.func_id == func_id && other.iteration == iteration);
  }

  DynamicFunction& operator=(const DynamicFunction& other) {
    func_id = other.func_id;
    iteration = other.iteration;
    return *this;
  }

  // Convert to a string representation (which is the old DDDG format).
  std::string str(SourceManager& srcManager) {
    const Function& func = srcManager.get<Function>(func_id);
    std::stringstream oss;
    oss << func.get_name() << "-" << iteration;
    return oss.str();
  }

  src_id_t get_function_id() const { return func_id; }
  unsigned get_iteration() const { return iteration; }

 private:
  src_id_t func_id;
  unsigned iteration;
};

extern const DynamicFunction InvalidDynamicFunction;

// Represents a dynamic reference to a variable.
//
// This pairs a Variable with a DynamicFunction (in which it is referred to).
class DynamicVariable {
 public:
  DynamicVariable() : dyn_func(InvalidDynamicFunction), var_id(-1) {}

  DynamicVariable(DynamicFunction& df, Variable& v)
      : dyn_func(df), var_id(v.get_id()) {}

  DynamicVariable(DynamicFunction& df, src_id_t var_id)
      : dyn_func(df), var_id(var_id) {}

  bool operator==(const DynamicVariable& other) const {
    return (dyn_func == other.dyn_func && other.var_id == var_id);
  }

  bool operator!=(const DynamicVariable& other) const {
    return !(this->operator==(other));
  }

  DynamicVariable& operator=(const DynamicVariable& other) {
    dyn_func = other.dyn_func;
    var_id = other.var_id;
    return *this;
  }

  DynamicFunction get_dynamic_function() const { return dyn_func; }
  src_id_t get_variable_id() const { return var_id; }

  // Convert to a string representation (which is the old DDDG format).
  std::string str(SourceManager& srcManager) {
    const Function& func = srcManager.get<Function>(dyn_func.get_function_id());
    const Variable& var = srcManager.get<Variable>(var_id);
    std::stringstream oss;
    oss << func.get_name() << "-" << dyn_func.get_iteration() << "-"
        << var.get_name();
    return oss.str();
  }

 private:
  DynamicFunction dyn_func;
  src_id_t var_id;
};

// Represents an instruction in a particular invocation of a function.
class DynamicInstruction {
 public:
  DynamicInstruction() : dyn_func(InvalidDynamicFunction), inst_id(-1) {}
  DynamicInstruction(DynamicFunction& df, src_id_t _inst_id)
      : dyn_func(df), inst_id(_inst_id) {}
  DynamicInstruction(DynamicFunction& df, Instruction& i)
      : dyn_func(df), inst_id(i.get_id()) {}

  bool operator==(const DynamicInstruction& other) const {
    return (dyn_func == other.dyn_func && inst_id == other.inst_id);
  }

  bool operator!=(const DynamicInstruction& other) const {
    return !(this->operator==(other));
  }

  DynamicInstruction& operator=(const DynamicInstruction& other) {
    dyn_func = other.dyn_func;
    inst_id = other.inst_id;
    return *this;
  }

  std::string str(SourceManager& srcManager) {
    const Instruction& inst = srcManager.get<Instruction>(inst_id);
    std::stringstream oss;
    oss << dyn_func.str(srcManager) << "-" << inst.get_name();
    return oss.str();
  }

  DynamicFunction get_dynamic_function() const { return dyn_func; }
  src_id_t get_inst_id() const { return inst_id; }

 private:
  DynamicFunction dyn_func;
  src_id_t inst_id;
};

extern const DynamicVariable InvalidDynamicVariable;

// Hashing functions to enable these classes to be stored as keys in hash maps.
namespace std {
template <> struct hash<DynamicFunction> {
  size_t operator()(const DynamicFunction& f) const {
    // Compute individual hash values for two data members and combine them
    // using XOR and bit shifting
    return (f.get_function_id() ^
            (std::hash<unsigned>()(f.get_iteration()) << 1) >> 1);
  }
};

template <> struct hash<DynamicVariable> {
  size_t operator()(const DynamicVariable& v) const {
    return (std::hash<DynamicFunction>()(v.get_dynamic_function()) ^
            (v.get_variable_id() << 2) >> 2);
  }
};

template <> struct hash<DynamicInstruction> {
  size_t operator()(const DynamicInstruction& di) const {
    const DynamicFunction& dynfunc = di.get_dynamic_function();
    return (std::hash<DynamicFunction>()(dynfunc) ^
            (di.get_inst_id() << 2) >> 2);
  }
};

};


#endif
