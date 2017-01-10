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

#include "SourceEntity.h"

extern const Function InvalidFunction;
extern const Variable InvalidVariable;
extern const Instruction InvalidInstruction;

// Central tracking data structure for all source code objects.
//
// The preferred way to access this structure is through the unique ids,
// because this only requires a single lookup, whereas name lookups require a
// second lookup to get the id.
//
// This class provides both read-only and read-write access to stored
// SourceEntities. The insert<T>() accessors provide non-const references
// (because adding a SourceEntity is a modifying operation), while all other
// accessors return const references. Unless the SourceEntity needs to be
// updated (for example, incrementing a function's invocation count), use the
// read-only accessors.
class SourceManager {
 public:
  SourceManager() {}

  /* Access and/or insert the SourceEntity of type T and the given name.
   *
   * If such an object does not exist, it is inserted first. A reference to the
   * object is returned.
   */
  template <class T> T& insert(std::string name);

  /* Access a SourceEntity of type T by name or id.
   *
   * If such an object does not exist, then an out of range exception is
   * thrown.
   */
  template <class T> const T& get(std::string& name);
  template <class T> const T& get(src_id_t id);

  /* Get the id of a SourceEntity of type T by name. */
  template <class T> src_id_t get_id(std::string& name);

 private:
  using src_id_map_t = std::map<std::string, src_id_t>;

  template <class T>
  T& add_or_get_source_entity(std::string& name,
                              src_id_map_t& name_to_id,
                              std::map<src_id_t, T>& id_to_entity) {
    auto it = name_to_id.find(name);
    if (it == name_to_id.end()) {
      T entity(name);
      name_to_id[name] = entity.get_id();
      id_to_entity[entity.get_id()] = entity;
      return id_to_entity[entity.get_id()];
    } else {
      return id_to_entity[it->second];
    }
  }

  template <class T>
  const T& get_source_entity(std::string& name,
                             src_id_map_t& name_to_id,
                             std::map<src_id_t, T>& id_to_entity) {
    auto it = name_to_id.find(name);
    if (it != name_to_id.end())
      return id_to_entity.at(it->second);
    return id_to_entity.at(InvalidId);
  }

  template <class T>
  const T& get_source_entity(src_id_t id,
                             std::map<src_id_t, T>& id_to_entity) {
    return id_to_entity.at(id);
  }

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
  std::map<src_id_t, Instruction> instructions;
  std::map<src_id_t, Label> labels;
  src_id_map_t fname_to_id;
  src_id_map_t vname_to_id;
  src_id_map_t iname_to_id;
  src_id_map_t lname_to_id;
};

template<> const Function& SourceManager::get<Function>(src_id_t id);
template<> const Variable& SourceManager::get<Variable>(src_id_t id);
template<> const Instruction& SourceManager::get<Instruction>(src_id_t id);

#endif
