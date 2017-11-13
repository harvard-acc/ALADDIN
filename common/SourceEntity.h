#ifndef _SOURCE_ENTITY_H_
#define _SOURCE_ENTITY_H_

/* Defines representations of static source code structures.
 *
 * Such structures include functions, variables, instructions, and labels. Each
 * of these basic structures is assigned a unique id and are stored inside a
 * SourceManager object.  IDs are unique across types, meaning that a function
 * and a variable could have the same C name but would still have different ids.
 */

#include <functional>
#include <iostream>
#include <sstream>
#include <string>

// Some type names here collide with other type names in libreadline.
namespace SrcTypes {

// It is unlikely for us to exceed 4 billion unique functions or variables.
using src_id_t = uint32_t;

// An ID that should never appear.
extern const src_id_t InvalidId;

class SourceManager;

// Common base class for all basic source code entities.
//
// Defines a name and id field and requires that child classes implement an
// set_id() function. All comparisons are done by comparing these IDs.
class SourceEntity {
  // Only the SourceManager will be allowed to construct these objects, thereby
  // letting us guarantee uniqueness and the ability to compare pointers.
  friend SourceManager;

 protected:
  SourceEntity() : name("") {}
  SourceEntity(std::string _name) : name(_name) {}
  virtual ~SourceEntity() {}

 public:
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
  friend SourceManager;

 protected:
  Function() : SourceEntity(), invocations(-1) {}
  Function(std::string _name) : SourceEntity(_name), invocations(-1) {
    set_id();
  }

 public:
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
  friend SourceManager;

 protected:
  Variable() : SourceEntity() {}
  Variable(std::string _name) : SourceEntity(_name) { set_id(); }

 public:
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
  friend SourceManager;

 protected:
  Instruction() : SourceEntity(), inductive(false) {}
  Instruction(std::string name) : SourceEntity(name) {
    set_id();
    inductive = (name.find("indvars") != std::string::npos);
  }

 public:
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
  friend SourceManager;

 protected:
  Label() : SourceEntity() {}
  Label(std::string name) : SourceEntity(name) { set_id(); }
  Label(unsigned line_num) : SourceEntity(std::to_string(line_num)) {
    set_id();
  }

 public:
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

class BasicBlock : public SourceEntity {
  friend SourceManager;

 protected:
  BasicBlock() : SourceEntity() {}
  BasicBlock(std::string name) : SourceEntity(name) { set_id(); }

 public:
  virtual std::string str() const {
    std::stringstream str;
    str << "BasicBlock(\"" << name << "\", id=" << id << ")";
    return str.str();
  }

 private:
  virtual void set_id() {
    std::hash<std::string> hash;
    id = hash("B_" + name);
  }
};

// A label that belongs to a function.
// TODO: Make this a SourceEntity that replaces the plain Label class.
class UniqueLabel {
 public:
  UniqueLabel() : function(nullptr), label(nullptr), line_number(0) {}
  UniqueLabel(Function* f, Label* l) : function(f), label(l), line_number(0) {}
  UniqueLabel(Function* f, Label* l, unsigned _line_num)
      : function(f), label(l), line_number(_line_num) {}

  explicit operator bool() const { return (function && label); }

  bool operator==(const UniqueLabel& other) const {
    return (*function == *other.function && *label == *other.label &&
            line_number == other.line_number);
  }

  Function* get_function() const { return function; }
  Label* get_label() const { return label; }
  int get_line_number() const { return line_number; }

  std::string str() const {
    std::stringstream str;
    str << "UniqueLabel(function=\"" << function->get_name()
        << "\", label=\"" << label->get_name() << "\", line_number="
        << line_number << ")";
    return str.str();
  }

 private:
  Function* function;
  Label* label;
  int line_number;
};

};  // namespace SrcTypes

// Hashing functions to enable these classes to be stored as keys in hash maps.
namespace std {
template <> struct hash<SrcTypes::UniqueLabel> {
  size_t operator()(const SrcTypes::UniqueLabel& l) const {
    if (l) {
      SrcTypes::src_id_t func_id = l.get_function()->get_id();
      SrcTypes::src_id_t label_id = l.get_label()->get_id();
      return (func_id ^ (label_id << 1) >> 1);
    }
    return SrcTypes::InvalidId;
  }
};

}
#endif
