#ifndef _MEMORY_AMBIGUATION_H_
#define _MEMORY_AMBIGUATION_H_

#include "base_opt.h"

/* A class to represent the sources of a memory address generation.
 *
 * An address to a memory operation is composed of a set of inductive operands
 * and a set of noninductive operands. A group of memory operations is
 * considered independent if they share the same set of non-inductive operands
 * and have the same number of inductive operands. Example:
 *
 *   for (int i = 0; i < 8; i++) {
 *     array[base + i] = some value;
 *     array[base][i] = some value;
 *   }
 *
 * This class wraps the underlying containers to hide the choice of
 * implementation from the user.
 */
class MemoryAddrSources {
 public:
  MemoryAddrSources() {}

  void add_noninductive(ExecNode* node) {
    noninductive.push_back(node);
  }

  void add_inductive(ExecNode* node) {
    inductive.push_back(node);
  }

  void merge(MemoryAddrSources&& other) {
    noninductive.merge(other.noninductive);
    inductive.merge(other.inductive);
  }

  bool empty() const {
    return noninductive.empty() && inductive.empty();
  }

  void print_noninductive() const {
    std::cout << "Noninductive: [ ";
    for (auto node : noninductive) {
      std::cout << node->get_node_id() << " ";
    }
    std::cout << "]\n";
  }

  void print_inductive() const {
    std::cout << "Inductive:    [ ";
    for (auto node : inductive) {
      std::cout << node->get_node_id() << " ";
    }
    std::cout << "]\n";
  }

  // Eliminate any duplicates and sort the nodes by node id, so that we can
  // then do a direct comparison between different MemoryAddrSources objects.
  void sort_and_uniquify() {
    auto compare = [](const ExecNode* n1, const ExecNode* n2) {
      return n1->get_node_id() < n2->get_node_id();
    };
    noninductive.sort(compare);
    noninductive.unique();
    inductive.sort(compare);
    inductive.unique();
  }

  // Returns true if this MemoryAddrSources object is independent of other, as
  // defined above.
  bool is_independent_of(const MemoryAddrSources& other, bool first=false) {
    bool is_independent = (noninductive == other.noninductive);
    // The first one may not have an IndexAdd node (since it uses the initial
    // value of the induction variable), so we only need to compare the
    // noninductive values.
    if (!first) {
      // We have to have at least one inductive node (otherwise the memory
      // addresses would actually be the same).
      is_independent &=
          (inductive.size() > 0 && inductive.size() == other.inductive.size());
    }
    return is_independent;
  }

 private:
  std::list<ExecNode*> noninductive;
  std::list<ExecNode*> inductive;
};

class MemoryAmbiguationOpt : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);

 protected:
  // Locate all sources of a GEP node.
  //
  // Starting from @current_node (which is initially the GEP node), recursively
  // examine each parent. Within the function @current_func, record all parents
  // that are either loads and index adds and return a MemoryAddrSources object.
  // If no such parents are found, return the oldest ancestor that is a
  // non-inductive compute op.
  MemoryAddrSources findMemoryAddrSources(ExecNode* current_node,
                                          SrcTypes::Function* current_func,
                                          EdgeNameMap& edge_to_parid);
};

#endif
