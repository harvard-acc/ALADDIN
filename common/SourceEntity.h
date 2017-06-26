#ifndef _SOURCE_ENTITY_H_
#define _SOURCE_ENTITY_H_

/* Defines representations of static source code structures.
 *
 * Such structures include functions, variables, instructions, and labels. Each
 * of these basic structures is assigned a unique id and are stored inside a
 * SourceManager object.  IDs are unique across types, meaning that a function
 * and a variable could have the same C name but would still have different ids.
 */

#include <iostream>
#include <sstream>
#include <string>

// Some type names here collide with other type names in libreadline.
namespace SrcTypes {

// It is unlikely for us to exceed 4 billion unique functions or variables.
using src_id_t = uint32_t;

// An ID that should never appear.
extern const src_id_t InvalidId;

// Common base class for all basic source code entities.
//
// Defines a name and id field and requires that child classes implement an
// set_id() function. All comparisons are done by comparing these IDs.
class SourceEntity {
 public:
  SourceEntity() : name("") {}
  SourceEntity(std::string _name) : name(_name) {}

  src_id_t get_id() const { return id; }
  const std::string& get_name() const { return name; }

  bool operator==(const SourceEntity& other) const { return (other.id == id); }
  bool operator!=(const SourceEntity& other) const {
    return !(this->operator==(other));
  }
  virtual std::string str() const = 0;
  virtual void dump() const { std::cout << str() << std::endl; }

 protected:
  virtual void set_id() = 0;

  std::string name;
  src_id_t id;
};

// A function in the source code.
class Function : public SourceEntity {
 public:
  Function() : SourceEntity(), invocations(-1) {}
  Function(std::string _name) : SourceEntity(_name), invocations(-1) {
    set_id();
  }

  void increment_invocations() { invocations++; }
  unsigned long get_invocations() const { return invocations; }
  virtual std::string str() const {
    std::stringstream str;
    str << "Function(\"" << name << "\", invocations=" << invocations
        << ", id=" << id << ")";
    return str.str();
  }

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

// A variable name.
//
// This could refer to a register or an array.
class Variable : public SourceEntity {
 public:
  Variable() : SourceEntity() {}
  Variable(std::string _name) : SourceEntity(_name) { set_id(); }
  virtual std::string str() const {
    std::stringstream str;
    str << "Variable(\"" << name << "\", id=" << id << ")";
    return str.str();
  }

 private:
  virtual void set_id() {
    std::hash<std::string> hash;
    id = hash("V_" + name);
  }
};

// A specifier for an instruction inside a basic block.
//
// The format of the specifier is determined by LLVM-Tracer.
class Instruction : public SourceEntity {
 public:
  Instruction() : SourceEntity(), inductive(false) {}
  Instruction(std::string name) : SourceEntity(name) {
    set_id();
    inductive = (name.find("indvars") != std::string::npos);
  }
  virtual std::string str() const {
    std::stringstream str;
    str << "Instruction(\"" << name << "\", id=" << id << ")";
    return str.str();
  }

  bool is_inductive() const { return inductive; }

 private:
  virtual void set_id() {
    std::hash<std::string> hash;
    id = hash("I_" + name);
  }

  bool inductive;
};

// The label of a labeled statement.
class Label : public SourceEntity {
 public:
  Label() : SourceEntity() {}
  Label(std::string name) : SourceEntity(name) { set_id(); }
  Label(unsigned line_num) : SourceEntity(std::to_string(line_num)) {
    set_id();
  }
  virtual std::string str() const {
    std::stringstream str;
    str << "Label(\"" << name << "\", id=" << id << ")";
    return str.str();
  }

 private:
  virtual void set_id() {
    std::hash<std::string> hash;
    id = hash("L_" + name);
  }
};

// A label that belongs to a function.
class UniqueLabel {
  public:
    UniqueLabel() : func_id(-1), label_id(-1) {}
    UniqueLabel(Function& f, Label& l)
        : func_id(f.get_id()), label_id(l.get_id()) {}
    UniqueLabel(src_id_t _func_id, src_id_t _label_id)
        : func_id(_func_id), label_id(_label_id) {}

    bool operator==(const UniqueLabel& other) const {
      return (func_id == other.func_id && label_id == other.label_id);
    }

    src_id_t get_function_id() const { return func_id; }
    src_id_t get_label_id() const { return label_id; }

   private:
    src_id_t func_id;
    src_id_t label_id;
};

};  // namespace SrcTypes

// Hashing functions to enable these classes to be stored as keys in hash maps.
namespace std {
template <> struct hash<SrcTypes::UniqueLabel> {
  size_t operator()(const SrcTypes::UniqueLabel& l) const {
    return (l.get_function_id() ^ (l.get_label_id() << 1) >> 1);
  }
};

}
#endif
