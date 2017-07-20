#include "SourceEntity.h"
#include "SourceManager.h"
#include "DynamicEntity.h"

#include <limits>

namespace SrcTypes {

const src_id_t InvalidId = std::numeric_limits<src_id_t>::max();

// Function overloads.
template<>
Function* SourceManager::get<Function>(const std::string& name) {
  return get_source_entity<Function>(name, fname_to_id, functions);
}

template<>
Function* SourceManager::get<Function>(src_id_t id) {
  return get_source_entity<Function>(id, functions);
}

template<>
Function* SourceManager::insert<Function>(std::string name) {
  return add_or_get_source_entity<Function>(name, fname_to_id, functions);
}

template<>
src_id_t SourceManager::get_id<Function>(const std::string& name) {
  return get_id(name, fname_to_id);
}

// Variable overloads
template<>
Variable* SourceManager::get<Variable>(const std::string& name) {
  return get_source_entity<Variable>(name, vname_to_id, variables);
}

template<>
Variable* SourceManager::get<Variable>(src_id_t id) {
  return get_source_entity<Variable>(id, variables);
}

template<>
Variable* SourceManager::insert<Variable>(std::string name) {
  return add_or_get_source_entity<Variable>(name, vname_to_id, variables);
}

template<>
src_id_t SourceManager::get_id<Variable>(const std::string& name) {
  return get_id(name, vname_to_id);
}

// Instruction overloads.
template<>
Instruction* SourceManager::get<Instruction>(const std::string& name) {
  return get_source_entity<Instruction>(name, iname_to_id, instructions);
}

template<>
Instruction* SourceManager::get<Instruction>(src_id_t id) {
  return get_source_entity<Instruction>(id, instructions);
}

template<>
Instruction* SourceManager::insert<Instruction>(std::string name) {
    return add_or_get_source_entity<Instruction>(name, iname_to_id, instructions);
}

template<>
src_id_t SourceManager::get_id<Instruction>(const std::string& name) {
  return get_id(name, iname_to_id);
}

// Label overloads
template<>
Label* SourceManager::get<Label>(const std::string& name) {
  return get_source_entity<Label>(name, lname_to_id, labels);
}

template<>
Label* SourceManager::get<Label>(src_id_t id) {
  return get_source_entity<Label>(id, labels);
}

template<>
Label* SourceManager::insert<Label>(std::string name) {
    return add_or_get_source_entity<Label>(name, lname_to_id, labels);
}

template<>
src_id_t SourceManager::get_id<Label>(const std::string& name) {
  return get_id(name, lname_to_id);
}

// Keeping the implementations of str() and dump() in the .cpp file is more
// likely to prevent the functions from being inlined (and thus not usable from
// the debugger).
std::string SourceManager::str(src_id_t id) {
  const Function* f = get<Function>(id);
  if (f) {
    return f->str();
  }
  const Variable* v = get<Variable>(id);
  if (v) {
    return v->str();
  }
  const Instruction* i = get<Instruction>(id);
  if (i) {
    return i->str();
  }
  const Label* l = get<Label>(id);
  if (l) {
    return l->str();
  }
  std::stringstream no_id;
  no_id << "No SourceEntity with id " << id << " found.";
  return no_id.str();
}

void SourceManager::dump(src_id_t id) {
    std::cout << str(id) << std::endl;
}

};

