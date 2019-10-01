#ifndef _LOOP_UNROLLING_H_
#define _LOOP_UNROLLING_H_

#include "base_opt.h"

// A loop boundary node is defined by static properties of the node and the
// current call and loop nesting level.
struct LoopBoundDescriptor {
  // Static properties.

  // The target basic block of this branch/call node.
  SrcTypes::BasicBlock* basic_block;
  // Either branch or call.
  unsigned microop;
  // How many function calls deep is this loop (from the top level function).
  unsigned call_depth;
  // The loop depth of the TARGET of this branch. We care not about the depth
  // of the branch itself, but where it is going to take us.
  unsigned loop_depth;

  // How many times have we iterated this loop?
  int dyn_invocations;

  LoopBoundDescriptor(SrcTypes::BasicBlock* _basic_block,
                      unsigned _microop,
                      unsigned _loop_depth,
                      unsigned _call_depth)
      : basic_block(_basic_block), microop(_microop), call_depth(_call_depth),
        loop_depth(_loop_depth), dyn_invocations(-1) {}

  // Returns true if @other is a branch that exits from this loop.
  bool exitBrIs(const LoopBoundDescriptor& other) {
    return (microop == other.microop && call_depth == other.call_depth &&
            loop_depth > other.loop_depth);
  }

  // Do not include dynamic properties in the equality check.
  bool operator==(const LoopBoundDescriptor& other) {
    return (basic_block == other.basic_block && microop == other.microop &&
            call_depth == other.call_depth);
  }

  bool operator!=(const LoopBoundDescriptor& other) {
    return !(*this == other);
  }

  friend std::ostream& operator<<(std::ostream& os,
                                  const LoopBoundDescriptor& obj);
};

class LoopUnrolling : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);
};

class LoopFlattening : public BaseAladdinOpt {
 public:
  using BaseAladdinOpt::BaseAladdinOpt;
  virtual void optimize();
  virtual std::string getCenteredName(size_t size);

};

#endif
