#include "SourceEntity.h"
#include "SourceManager.h"
#include "DynamicEntity.h"

#include <limits>

namespace SrcTypes {

const src_id_t InvalidId = std::numeric_limits<src_id_t>::max();

template<>
std::string SourceManager::get_type_prefix<Function>() const {
  return "F";
};

template<>
std::string SourceManager::get_type_prefix<Variable>() const {
  return "V";
};

template<>
std::string SourceManager::get_type_prefix<Instruction>() const {
  return "I";
};

template<>
std::string SourceManager::get_type_prefix<Label>() const {
  return "L";
};

template<>
std::string SourceManager::get_type_prefix<BasicBlock>() const {
  return "B";
};

};

