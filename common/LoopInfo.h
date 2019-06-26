#ifndef _SAMPLING_H_
#define _SAMPLING_H_

#include "DynamicEntity.h"
#include "ExecNode.h"
#include "SourceEntity.h"
#include "typedefs.h"

class Program;

// This is a tree structure that tries to capture the hierarchy of sampled
// loops. The hierarchy assumes regular nested loops, not loops that break out
// to parent loops or do any other kinds of irregular control flow.
class SampledLoop {
 public:
  SampledLoop(const SrcTypes::DynamicLabel* _label,
              float _factor,
              int _start_cycle,
              int _end_cycle,
              int _start_node_id,
              int _end_node_id)
      : label(_label), factor(_factor), start_cycle(_start_cycle),
        end_cycle(_end_cycle), elapsed_cycle(_end_cycle - _start_cycle),
        start_node_id(_start_node_id), end_node_id(_end_node_id),
        upsampled(false), parent(nullptr) {}

  bool contains(const SampledLoop* other) const {
    return start_node_id <= other->start_node_id &&
           end_node_id >= other->end_node_id;
  }

  bool isRootNode() const {
    // The root node is contrived by Aladdin, which doesn't have a label.
    return label == nullptr;
  }

  std::string str() const {
    std::stringstream ss;
    if (isRootNode())
      ss << "Root node, ";
    else
      ss << label->str() << ", ";
    ss << "factor: " << factor << ", cycles: [" << start_cycle << ", "
       << end_cycle << "], upsampled: " << upsampled
       << ", elapsed cycle: " << elapsed_cycle << ", node IDs: ["
       << start_node_id << ", " << end_node_id << "]";
    return ss.str();
  }

  float factor;
  int start_cycle;
  int end_cycle;
  int elapsed_cycle;
  unsigned int start_node_id;
  unsigned int end_node_id;
  SampledLoop* parent;
  std::vector<SampledLoop*> children;
  bool upsampled;

 private:
  const SrcTypes::DynamicLabel* label;
};

class LoopSampling {
 public:
  LoopSampling(Program* _program) : program(_program) {
    insertRootNode();
  }

  ~LoopSampling() {
    for (auto s : loop_samples)
      delete s;
  }

  void addSamplingFactor(SrcTypes::DynamicLabel& label, float factor) {
    loop_sampling_factors[label] = factor;
  }

  void clearSamplingInfo() {
    loop_sampling_factors.clear();
    for (auto s : loop_samples)
      delete s;
    loop_samples.clear();
    // Insert a new root node for the next accelerator invocation.
    insertRootNode();
  }

  // This updates the line numbers of the sampled loops using the labelmap.
  void updateSamplingWithLabelInfo();

  // Upsample the sampled loop latencies. This returns the correction cycles
  // to add to num_cycles.
  int upsampleLoops();

  // Print the details of the sample tree.
  void printSampleTree();

 private:
  // Create a root node that has infinite length, ensuring that all loop
  // samples will be descendents of it.
  void insertRootNode() {
    root = new SampledLoop(nullptr, 1, 0, std::numeric_limits<int>::max(), 0,
                           std::numeric_limits<int>::max());
    root->elapsed_cycle = 0;
    loop_samples.push_back(root);
  }

  // Build the sample tree by inserting all the sampled loops
  void buildSampleTree();

  // This is a recursive function that searches for the best place to insert the
  // sample at. The search begins downwards from the given node.
  bool insertSample(SampledLoop* node, SampledLoop* sample);

  // Multiply elapsed time of this node and all children by factor.
  void upsampleChildren(SampledLoop* node, float factor);

  // Update the tree by upsampling this node's execution time.
  void upsampleParents(SampledLoop* node, int correction_cycle);

  // Correct the sample's cycles due to DMA operations.
  void sampleDmaCorrection(SampledLoop* sample);

  void printSampleTree(SampledLoop* node, int level);

 private:
  // Dynamic loop samples.
  std::vector<SampledLoop*> loop_samples;
  // Sampled loop labels and sampling factors.
  std::unordered_map<SrcTypes::DynamicLabel, float> loop_sampling_factors;
  SampledLoop* root;
  Program* program;
};

#endif
