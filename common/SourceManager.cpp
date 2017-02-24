#include "SourceEntity.h"
#include "SourceManager.h"
#include "DynamicEntity.h"

#include <limits>

const src_id_t InvalidId = std::numeric_limits<src_id_t>::max();

// Variable names can't possibly have spaces, so this is a safe way to make it
// clear that these objects are invalid (particularly when printed out by
// dump()).
DECLARE_INVALID_SOURCE_ENTITY(Function, "This is an invalid SourceEntity");
DECLARE_INVALID_SOURCE_ENTITY(Variable, "This is an invalid SourceEntity");
DECLARE_INVALID_SOURCE_ENTITY(Instruction, "This is an invalid SourceEntity");
DECLARE_INVALID_SOURCE_ENTITY(Label, "This is an invalid SourceEntity");

// These are not SourceEntity objects.
const DynamicFunction InvalidDynamicFunction;
const DynamicVariable InvalidDynamicVariable;

// Function overloads.
template<>
const Function& SourceManager::get<Function>(const std::string& name) {
  return static_cast<const Function&>(get_source_entity<Function>(
        name, fname_to_id, functions));
}

template<>
const Function& SourceManager::get<Function>(src_id_t id) {
  return static_cast<const Function&>(get_source_entity<Function>(
        id, functions));
}

template<>
Function& SourceManager::insert<Function>(std::string name) {
  return static_cast<Function&>(
      add_or_get_source_entity<Function>(name, fname_to_id, functions));
}

template<>
src_id_t SourceManager::get_id<Function>(const std::string& name) {
  return get_id(name, fname_to_id);
}

// Variable overloads
template<>
const Variable& SourceManager::get<Variable>(const std::string& name) {
  return static_cast<const Variable&>(get_source_entity<Variable>(
        name, vname_to_id, variables));
}

template<>
const Variable& SourceManager::get<Variable>(src_id_t id) {
  return static_cast<const Variable&>(get_source_entity<Variable>(
        id, variables));
}

template<>
Variable& SourceManager::insert<Variable>(std::string name) {
  return static_cast<Variable&>(
      add_or_get_source_entity<Variable>(name, vname_to_id, variables));
}

template<>
src_id_t SourceManager::get_id<Variable>(const std::string& name) {
  return get_id(name, vname_to_id);
}

// Instruction overloads.
template<>
const Instruction& SourceManager::get<Instruction>(const std::string& name) {
  return static_cast<const Instruction&>(get_source_entity<Instruction>(
        name, iname_to_id, instructions));
}

template<>
const Instruction& SourceManager::get<Instruction>(src_id_t id) {
  return static_cast<const Instruction&>(get_source_entity<Instruction>(
        id, instructions));
}

template<>
Instruction& SourceManager::insert<Instruction>(std::string name) {
    return static_cast<Instruction&>(
        add_or_get_source_entity<Instruction>(name, iname_to_id, instructions));
}

template<>
src_id_t SourceManager::get_id<Instruction>(const std::string& name) {
  return get_id(name, iname_to_id);
}

// Label overloads
template<>
const Label& SourceManager::get<Label>(const std::string& name) {
  return static_cast<const Label&>(get_source_entity<Label>(
        name, lname_to_id, labels));
}

template<>
const Label& SourceManager::get<Label>(src_id_t id) {
  return static_cast<const Label&>(get_source_entity<Label>(
        id, labels));
}

template<>
Label& SourceManager::insert<Label>(std::string name) {
    return static_cast<Label&>(
        add_or_get_source_entity<Label>(name, lname_to_id, labels));
}

template<>
src_id_t SourceManager::get_id<Label>(const std::string& name) {
  return get_id(name, lname_to_id);
}

// Keeping the implementations of str() and dump() in the .cpp file is more
// likely to prevent the functions from being inlined (and thus not usable from
// the debugger).
std::string SourceManager::str(src_id_t id) {
  const Function& f = get<Function>(id);
  if (f != InvalidFunction) {
    return f.str();
  }
  const Variable& v = get<Variable>(id);
  if (v != InvalidVariable) {
    return v.str();
  }
  const Instruction& i = get<Instruction>(id);
  if (i != InvalidInstruction) {
    return i.str();
  }
  const Label& l = get<Label>(id);
  if (l != InvalidLabel) {
    return l.str();
  }
  std::stringstream no_id;
  no_id << "No SourceEntity with id " << id << " found.";
  return no_id.str();
}

void SourceManager::dump(src_id_t id) {
    std::cout << str(id) << std::endl;
}

