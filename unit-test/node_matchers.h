#ifndef _UNIT_TEST_NODE_MATCHERS_
#define _UNIT_TEST_NODE_MATCHERS_
#include <sstream>

#include "catch.hpp"
#include "ExecNode.h"
#include "opcode_func.h"

namespace aladdin {
namespace testing {

// ContainsConnectedNodes matches an opcode against a list of node ids. It
// returns true if any of those nodes are of the indicated type. Optionally, it
// takes a number indicating how many such opcodes it should find in the set
// (e.g.  contains two branches). Defaults to searching for any.
class ContainsConnectedNodes : public Catch::MatcherBase<Program> {
  public:
   ContainsConnectedNodes(uint8_t opcode,
                          unsigned node_id,
                          int num_matches = kMatchAnyNumber)
       : opcode_(opcode), node_id_(node_id), num_matches_(num_matches) {}

   bool match_any() const {
     return num_matches_ == kMatchAnyNumber;
   }

   bool match(const Program& prog) const override {
     std::vector<unsigned> connected_nodes = prog.getConnectedNodes(node_id_);
     int matches_found = 0;
     for (unsigned node_id : connected_nodes) {
       if (prog.nodes.at(node_id)->get_microop() == opcode_) {
         matches_found++;
       }
     }
     if (match_any()) return matches_found > 0;
     return (num_matches_ == matches_found);
  }

  virtual std::string describe() const override {
    std::stringstream ss;
    if (match_any()) {
      ss << "node " << node_id_
         << " is connected to at least one node(s) of type "
         << opcode_name(opcode_);
    } else {
      ss << "node " << node_id_ << " is connected to " << num_matches_
         << " node(s) of type " << opcode_name(opcode_);
    }
    return ss.str();
  }

  // An internal value indicating that we should match against any number of
  // opcodes.
  static constexpr int kMatchAnyNumber = -1;

  private:
    uint8_t opcode_;
    unsigned node_id_;
    int num_matches_;
};

// Registers the matcher named Contains[OpType]ConnectedTo, which takes a
// node_id and optionally a number of nodes as an argument.
//
// Examples:
//   // Asserts that there is at least one branch connected to node_id.
//   REQUIRE_THAT(prog, ContainsBranchConnectedTo(node_id));
//   // Asserts that there are exactly four stores connected to node_id.
//   REQUIRE_THAT(prog, ContainsStoreConnectedTo(node_id, 4));
#define DECLARE_CONNECTED_MATCHER(OpType, Opcode)                              \
  class ContainsConnectedNode##OpType##Type : public ContainsConnectedNodes {  \
   public:                                                                     \
    ContainsConnectedNode##OpType##Type(unsigned node_id, int num_matches)     \
        : ContainsConnectedNodes(Opcode, node_id, num_matches) {}              \
  };                                                                           \
  ContainsConnectedNode##OpType##Type Contains##OpType##ConnectedTo(           \
      unsigned node_id,                                                        \
      int num_matches = ContainsConnectedNodes::kMatchAnyNumber) {             \
    return ContainsConnectedNode##OpType##Type(node_id, num_matches);          \
  }

DECLARE_CONNECTED_MATCHER(Branch, LLVM_IR_Br);
DECLARE_CONNECTED_MATCHER(Load, LLVM_IR_Load);
DECLARE_CONNECTED_MATCHER(Store, LLVM_IR_Store);
DECLARE_CONNECTED_MATCHER(DMALoad, LLVM_IR_DMALoad);
DECLARE_CONNECTED_MATCHER(DMAStore, LLVM_IR_DMAStore);

}  // namespace testing
}  // namespace aladdin

#endif
