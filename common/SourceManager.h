#ifndef _SOURCE_MANAGER_H_
#define _SOURCE_MANAGER_H_

/* Manager and storage for language constructs.
 *
 * SourceManager tracks all functions and variables that are observed in the
 * dynamic trace, including invocation counts and other metadata, and provides
 * efficient means to manipulate these objects via unique IDs, which is obtained
 * by hashing the individual fields. This precludes the need to perform
 * expensive string comparisons and concatenations.
 */

#include <limits>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "SourceEntity.h"

namespace SrcTypes {

// Central tracking data structure for all source code objects.
//
// The preferred way to access this structure is through the unique ids,
// because this only requires a single lookup, whereas name lookups require a
// second lookup to get the id.
//
// This class provides both read-only and read-write access to stored
// SourceEntities. The insert<T>() and get<T>() accessors return pointers to
// the added or retrieved objects.
class SourceManager {
 public:
  SourceManager() {
    name_to_id["F"] = src_id_map_t();
    name_to_id["V"] = src_id_map_t();
    name_to_id["I"] = src_id_map_t();
    name_to_id["L"] = src_id_map_t();
    name_to_id["B"] = src_id_map_t();
  }

  ~SourceManager() {
    for (auto pair : source_entities) {
      delete pair.second;
    }
    source_entities.clear();
    name_to_id.clear();
  }

  /* Access a SourceEntity of type T by name or id.
   *
   * If such an object does not exist, then a nullptr is returned.
   */
  template <class T>
  T* get(const std::string& name) const {
    std::string prefix = get_type_prefix<T>();
    const src_id_map_t& _map = name_to_id.at(prefix);
    auto it = _map.find(name);
    if (it == _map.end())
      return nullptr;
    src_id_t id = it->second;
    // No need to check for existence here. If a mapping exists from name to
    // id, then there must be an object by that id.
    return static_cast<T*>(source_entities.at(id));
  }

  template <class T>
  T* get(src_id_t id) {
    auto it = source_entities.find(id);
    if (it == source_entities.end())
      return nullptr;
    return static_cast<T*>(it->second);
  }

  /* Access and/or insert the SourceEntity of type T and the given name.
   *
   * If such an object does not exist, it is inserted first. A pointer to the
   * object is returned.
   */
  template <class T>
  T* insert(std::string name) {
    T* obj = get<T>(name);
    if (obj)
      return obj;

    std::string prefix = get_type_prefix<T>();
    T* entity = new T(name);
    src_id_t id = entity->get_id();
    name_to_id.at(prefix)[name] = id;
    source_entities[id] = entity;
    return entity;
  }

  /* Get the id of a SourceEntity of type T by name. */
  template <class T>
  src_id_t get_id(const std::string& name) {
    std::string prefix = get_type_prefix<T>();
    src_id_map_t& _map = name_to_id[prefix];
    auto it = _map.find(name);
    if (it == _map.end())
      return it->second;
    return InvalidId;
  }

 private:
  using src_id_map_t = std::map<std::string, src_id_t>;

  /* Return a unique prefix for a SourceEntity type.
   *
   * This is used to distinguish the same name for different SourceEntity
   * types.
   */
  template <class T>
  std::string get_type_prefix() const;

  // A unified map from ID to SourceEntity.
  //
  // All IDs for valid SourceEntity objects are guaranteed to be unique, even
  // across types.
  std::map<src_id_t, SourceEntity*> source_entities;

  // First key is a name prefix indicating the type.
  std::map<std::string, src_id_map_t> name_to_id;
};

};
#endif
