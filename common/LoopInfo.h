#ifndef _LOOP_INFO_H_
#define _LOOP_INFO_H_

#include "DynamicEntity.h"
#include "ExecNode.h"
#include "SourceEntity.h"
#include "typedefs.h"

class Program;

// This is a tree structure that tries to capture the hierarchy of loop
// iterations. The hierarchy assumes regular nested loops, not loops that break
// out to parent loops or do any other kinds of irregular control flow.
class LoopIteration {
 public:
  LoopIteration(const SrcTypes::UniqueLabel* _label = nullptr,
                const ExecNode* _start_node = nullptr,
                const ExecNode* _end_node = nullptr)
      : label(_label), start_node(_start_node), end_node(_end_node),
        parent(nullptr), pipelined(false), sampled(false), factor(1),
        upsampled(false), elapsed_cycle(0) {}

  bool contains(const LoopIteration* other) const {
    return isRootNode() ||
           (start_node->get_node_id() <= other->start_node->get_node_id() &&
            end_node->get_node_id() >= other->end_node->get_node_id());
  }

  bool isRootNode() const {
    // The root node is contrived by Aladdin, which doesn't have a label.
    return label == nullptr;
  }

  std::string str() const {
    std::stringstream ss;
    if (isRootNode()) {
      ss << "Root node, elapsed cycle: " << elapsed_cycle;
    } else {
      ss << label->str() << ", node IDs: [" << start_node->get_node_id() << ", "
         << end_node->get_node_id() << "], cycles: ["
         << start_node->get_complete_execution_cycle() << ", "
         << end_node->get_complete_execution_cycle()
         << "], pipelined: " << pipelined << ", sampled: " << sampled
         << ", factor: " << factor << ", elapsed cycle: " << elapsed_cycle;
    }
    return ss.str();
  }

  const SrcTypes::UniqueLabel* label;
  const ExecNode* start_node;
  const ExecNode* end_node;
  LoopIteration* parent;
  std::vector<LoopIteration*> children;
  bool pipelined;

  // Fields specific to sampling.
  bool sampled;
  float factor;
  bool upsampled;
  int elapsed_cycle;
};

class LoopInfo {
 public:
  LoopInfo(const Program* _program) : program(_program) {}

  ~LoopInfo() {
    for (auto s : loop_iters)
      delete s;
  }

  //=------------ Loop tree operations ---------------=//

  // Build the loop tree by inserting all the loop iterations.
  void buildLoopTree();

  // Print the details of the loop tree.
  void printLoopTree();

  LoopIteration* getRootNode() const { return root; }

  void clear() {
    loop_sampling_factors.clear();
    for (auto l : loop_iters)
      delete l;
    loop_iters.clear();
  }

  //=------------ Sampling functions ---------------=//

  void addSamplingFactor(SrcTypes::DynamicLabel& label, float factor) {
    loop_sampling_factors[label] = factor;
  }

  // This updates the line numbers of the sampled loops using the labelmap.
  void updateSamplingWithLabelInfo();

  // Upsample the sampled loop latencies. This returns the correction cycles
  // to add to num_cycles.
  int upsampleLoops();

 private:
  //=------------ Loop tree operations ---------------=//

  // Create a root node that all loop iterations will be descendents of.
  void insertRootNode() {
    root = new LoopIteration();
    loop_iters.push_back(root);
  }

  // This is a recursive function that searches for the best place to insert the
  // loop iteration at. The search begins downwards from the given node.
  bool insertLoop(LoopIteration* node, LoopIteration* loop);

  void printLoopTree(LoopIteration* node, int level);

  //=------------ Sampling functions ---------------=//

  // Update the root node by upsampling this node's execution time upwards.
  void upsampleLoop(LoopIteration* node, int correction_cycle);

  // Correct the sample's cycles due to DMA operations.
  void sampleDmaCorrection(LoopIteration* loop);

 private:
  // Dynamic loop iterations.
  std::vector<LoopIteration*> loop_iters;
  // Sampled loop labels and sampling factors.
  std::unordered_map<SrcTypes::DynamicLabel, float> loop_sampling_factors;
  LoopIteration* root;
  const Program* program;
};

#endif
