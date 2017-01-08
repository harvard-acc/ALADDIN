#ifndef _SOURCE_CODE_STRUCTS_H_
#define _SOURCE_CODE_STRUCTS_H_

/* Manager and storage for language constructs.
 *
 * SourceManager tracks all functions and variables that are observed in the
 * dynamic trace, including invocation counts and other metadata, and provides
 * efficient means to manipulate these objects via unique IDs, which is obtained
 * by hashing the individual fields. This precludes the need to perform
 * expensive string comparisons and concatenations.
 */

#include <limits>
#include <map>
#include <sstream>
#include <string>

// It is unlikely for us to exceed 4 billion unique functions or variables.
using src_id_t = uint32_t;

// Common base class for all basic source code entities.
//
// Defines a name and id field and requires that child classes implement an
// set_id() function. All comparisons are done by comparing these IDs.
class SourceEntity {
 public:
  SourceEntity() : name("") {}
  SourceEntity(std::string _name) : name(_name) {}

  src_id_t get_id() const { return id; }
  const std::string get_name() const { return name; }

  bool operator==(const SourceEntity& other) const { return (other.id == id); }

 protected:
  virtual void set_id() = 0;

  std::string name;
  src_id_t id;
};

class Function : public SourceEntity {
 public:
  Function() : SourceEntity(), invocations(-1) {}
  Function(std::string _name) : SourceEntity(_name), invocations(-1) {
    set_id();
  }

  void increment_invocations() { invocations++; }
  unsigned long get_invocations() const { return invocations; }

 private:
  // To prevent hash collisions between functions and variables with the same
  // name, append a prefix.
  virtual void set_id() {
    std::hash<std::string> hash;
    id = hash("F_" + name);
  }

  // The number of times this function has been called thus far.
  // It does NOT refer to a particular invocation - for that, see DynamicFunction.
  long invocations;
};

class Variable : public SourceEntity {
 public:
  Variable() : SourceEntity() {}
  Variable(std::string _name) : SourceEntity(_name) { set_id(); }

 private:
  virtual void set_id() {
    std::hash<std::string> hash;
    id = hash("V_" + name);
  }
};

extern const Function InvalidFunction;
extern const Variable InvalidVariable;

// Central tracking data structure for all source code objects.
//
// The preferred way to access this structure is through the unique ids,
// because this only requires a single lookup, whereas name lookups require a
// second lookup to get the id.
//
// This class provides both read-only and read-write access to stored
// SourceEntities. The add_or_get_*() accessors provide non-const references
// (because adding a SourceEntity is a modifying operation), while all other
// accessors return const references. Unless the SourceEntity needs to be
// updated (for example, incrementing a function's invocation count), use the
// read-only accessors.
class SourceManager {
 public:
  SourceManager() {}

  Function& add_or_get_function(std::string name) {
    auto it = fname_to_id.find(name);
    if (it == fname_to_id.end()) {
      Function f(name);
      fname_to_id[name] = f.get_id();
      functions[f.get_id()] = f;
      return functions[f.get_id()];
    } else {
      return functions[it->second];
    }
  }

  Variable& add_or_get_variable(std::string name) {
    auto it = vname_to_id.find(name);
    if (it == vname_to_id.end()) {
      Variable v(name);
      vname_to_id[name] = v.get_id();
      variables[v.get_id()] = v;
      return variables[v.get_id()];
    } else {
      return variables[it->second];
    }
  }

  const Function& get_function(std::string name) {
    auto it = fname_to_id.find(name);
    if (it == fname_to_id.end())
      return InvalidFunction;
    return functions[it->second];
  }

  const Variable& get_variable(std::string name) {
    auto it = fname_to_id.find(name);
    if (it == fname_to_id.end())
      return InvalidVariable;
    return variables[it->second];
  }

  const Variable& get_variable(src_id_t id) {
    auto it = variables.find(id);
    if (it == variables.end())
      return InvalidVariable;
    return it->second;
  }

  const Function& get_function(src_id_t id) {
    auto it = functions.find(id);
    if (it == functions.end())
      return InvalidFunction;
    return it->second;
  }

  src_id_t get_function_id(std::string func_name) {
    return get_id(func_name, fname_to_id);
  }

  src_id_t get_variable_id(std::string var_name) {
    return get_id(var_name, vname_to_id);
  }

 private:
  using src_id_map_t = std::map<std::string, src_id_t>;

  src_id_t get_id(std::string& name, src_id_map_t& _map) {
    auto it = _map.find(name);
    if (it == _map.end())
      return InvalidId;
    return it->second;
  }

  // Different object types are stored in separate containers so that lookups
  // can be performed directly by id or name, without needing to append any
  // string prefixes.
  std::map<src_id_t, Function> functions;
  std::map<src_id_t, Variable> variables;
  src_id_map_t fname_to_id;
  src_id_map_t vname_to_id;

  src_id_t InvalidId = std::numeric_limits<src_id_t>::max();
};

// Represents a specific invocation of a function.
//
// This pairs a Function with an iteration. For efficiency, this stores
// the function id instead of a reference to the Function object itself.
//
// This is NOT a SourceEntity because it only exists in the context of a
// specific execution.
class DynamicFunction {
 public:
  DynamicFunction() : func_id(-1), iteration(-1) {}
  DynamicFunction(Function& f)
      : func_id(f.get_id()), iteration(f.get_invocations()) {}
  DynamicFunction(Function& f, unsigned it)
      : func_id(f.get_id()), iteration(it) {}

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
    const Function& func = srcManager.get_function(func_id);
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
  DynamicVariable() : dynfunc(InvalidDynamicFunction), var_id(-1) {}

  DynamicVariable(DynamicFunction& df, Variable& v)
      : dynfunc(df), var_id(v.get_id()) {}

  bool operator==(const DynamicVariable& other) const {
    return (other.dynfunc == dynfunc && other.var_id == var_id);
  }

  bool operator!=(const DynamicVariable& other) const {
    return !(this->operator==(other));
  }

  DynamicVariable& operator=(const DynamicVariable& other) {
    dynfunc = other.dynfunc;
    var_id = other.var_id;
    return *this;
  }

  const DynamicFunction& get_dynamic_function() const { return dynfunc; }
  src_id_t get_variable_id() const { return var_id; }

  // Convert to a string representation (which is the old DDDG format).
  std::string str(SourceManager& srcManager) {
    const Function& func = srcManager.get_function(dynfunc.get_function_id());
    const Variable& var = srcManager.get_variable(var_id);
    std::stringstream oss;
    oss << func.get_name() << "-" << dynfunc.get_iteration() << "-"
        << var.get_name();
    return oss.str();
  }

 private:
  DynamicFunction dynfunc;
  src_id_t var_id;
};

extern const DynamicVariable InvalidDynamicVariable;

// Hashing functions to enable these classes to be stored in STL unordered maps.
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
    const DynamicFunction& dynfunc = v.get_dynamic_function();
    return (std::hash<DynamicFunction>()(dynfunc) ^
            (v.get_variable_id() << 2) >> 2);
  }
};
};

#endif
