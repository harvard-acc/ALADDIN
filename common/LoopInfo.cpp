#include <limits>

#include "Sampling.h"
#include "Program.h"

void LoopSampling::updateSamplingWithLabelInfo() {
  const labelmap_t& labelmap = program->labelmap;
  const inline_labelmap_t& inline_labelmap = program->inline_labelmap;
  // Because we store the dynamic labels for the sampled loops, to update the
  // line numbers, here we need to find all that match the static label we get
  // from the label map. Same for the inlined loops below.
  for (auto it = labelmap.begin(); it != labelmap.end(); ++it) {
    const SrcTypes::UniqueLabel& label_with_num = it->second;
    std::vector<std::pair<SrcTypes::DynamicLabel, float>>
        to_add_sampling_factors;
    auto sample_it = loop_sampling_factors.begin();
    while (sample_it != loop_sampling_factors.end()) {
      auto sample_label = sample_it->first;
      if (sample_label.get_dynamic_function()->get_function() ==
              label_with_num.get_function() &&
          sample_label.get_label() == label_with_num.get_label()) {
        float factor = sample_it->second;
        SrcTypes::DynamicLabel dyn_label = sample_label;
        dyn_label.set_line_number(label_with_num.get_line_number());
        to_add_sampling_factors.push_back(std::make_pair(dyn_label, factor));
        sample_it = loop_sampling_factors.erase(sample_it);
      } else {
        ++sample_it;
      }
    }
    for (auto& sample : to_add_sampling_factors)
      loop_sampling_factors[sample.first] = sample.second;
  }

  for (auto it = inline_labelmap.begin(); it != inline_labelmap.end(); ++it) {
    const SrcTypes::UniqueLabel& inlined_label = it->first;
    const SrcTypes::UniqueLabel& orig_label = it->second;
    std::vector<std::pair<SrcTypes::DynamicLabel, float>>
        to_add_sampling_factors;
    auto sample_it = loop_sampling_factors.begin();
    while (sample_it != loop_sampling_factors.end()) {
      auto sample_label = sample_it->first;
      if (sample_label.get_dynamic_function()->get_function() ==
              orig_label.get_function() &&
          sample_label.get_label() == orig_label.get_label()) {
        float factor = sample_it->second;
        SrcTypes::DynamicLabel dyn_label = sample_label;
        dyn_label.set_line_number(inlined_label.get_line_number());
        to_add_sampling_factors.push_back(std::make_pair(dyn_label, factor));
        sample_it = loop_sampling_factors.erase(sample_it);
      } else {
        ++sample_it;
      }
    }
    for (auto& sample : to_add_sampling_factors)
      loop_sampling_factors[sample.first] = sample.second;
  }
}

void LoopSampling::buildSampleTree() {
  for (auto& label_factor : loop_sampling_factors) {
    std::list<cnode_pair_t> loop_bound_nodes =
        program->findLoopBoundaries(label_factor.first);
    for (auto& bound : loop_bound_nodes) {
      const ExecNode* first = bound.first;
      const ExecNode* second = bound.second;
      const SrcTypes::DynamicLabel* label = &label_factor.first;
      float factor = label_factor.second;
      SampledLoop* sample =
          new SampledLoop(label, factor, first->get_complete_execution_cycle(),
                          second->get_complete_execution_cycle(),
                          first->get_node_id(), second->get_node_id());
      sampleDmaCorrection(sample);
      loop_samples.push_back(sample);
      insertSample(root, sample);
    }
  }
}

bool LoopSampling::insertSample(SampledLoop* node, SampledLoop* sample) {
  if (node->contains(sample)) {
    // The sample should be inserted to the subtree from this node. We first
    // check if the node is a leaf node and directly insert the sample if so.
    // Otherwise, we look at every child of this node and see if the sample
    // will become a new parent of it (i.e., the sample will be inserted between
    // the node and any of its children). If not, we try to insert the sample to
    // the subtree from that child.
    if (node->children.size() == 0) {
      node->children.push_back(sample);
      sample->parent = node;
      return true;
    }
    bool inserted = false;
    auto child_it = node->children.begin();
    while (child_it != node->children.end()) {
      if (sample->contains(*child_it)) {
        sample->parent = node;
        sample->children.push_back(*child_it);
        (*child_it)->parent = sample;
        child_it = node->children.erase(child_it);
        if (std::find(node->children.begin(), node->children.end(), sample) ==
            node->children.end()) {
          child_it = node->children.insert(child_it, sample);
          ++child_it;
        }
        inserted = true;
      } else {
        inserted |= insertSample(*child_it, sample);
        ++child_it;
      }
    }
    if (!inserted) {
      // The sample is neither a child nor a parent of any children, thus it's a
      // new sibling.
      node->children.push_back(sample);
      sample->parent = node;
    }
    return true;
  } else {
    // The sample can't be inserted into the path downward from the node. Since
    // the root node has an infinite interval, we are guaranteed that the sample
    // can be inserted to the tree.
    return false;
  }
}

void LoopSampling::upsampleChildren(SampledLoop* node, float factor) {
  node->elapsed_cycle *= factor;
  for (auto& child : node->children)
    upsampleChildren(child, factor);
}

void LoopSampling::upsampleParents(SampledLoop* node, int correction_cycle) {
  if (node->parent) {
    node->parent->elapsed_cycle += correction_cycle;
    upsampleParents(node->parent, correction_cycle);
  }
}

int LoopSampling::upsampleLoops() {
  buildSampleTree();
  for (auto& sample : loop_samples) {
    int correction_cycle = sample->elapsed_cycle * (sample->factor - 1);
    upsampleChildren(sample, sample->factor);
    upsampleParents(sample, correction_cycle);
    sample->upsampled = true;
  }
  return root->elapsed_cycle;
}

void LoopSampling::sampleDmaCorrection(SampledLoop* sample) {
  // Check every node between this loop boundary to see if it has a DMA
  // load parent. This is due to that we don't make any subsequent
  // operations block on a DMA operation (as long as there's no memory
  // dependency), so that a loop afterwards can start executing before the
  // DMA operation. As a result, the loop latency would include the DMA
  // latency, which is not intended for sampling purpose. Here, as a
  // heuristic approach, we will use the DMA completion time as the start
  // cycle for the loop, based on the fact that DMA operations normally
  // have long latencies and we tend to use the DMA data right away as we
  // enter the loop.
  std::list<std::pair<int, int>> dma_intervals;
  for (unsigned int node_id = sample->start_node_id + 1;
       node_id < sample->end_node_id;
       node_id++) {
    ExecNode* node = program->nodes.at(node_id);
    in_edge_iter in_edge_it, in_edge_end;
    for (boost::tie(in_edge_it, in_edge_end) =
             in_edges(node->get_vertex(), program->graph);
         in_edge_it != in_edge_end;
         ++in_edge_it) {
      Vertex parent_vertex = source(*in_edge_it, program->graph);
      ExecNode* parent_node = program->nodeAtVertex(parent_vertex);
      if (parent_node->is_dma_load()) {
        dma_intervals.push_back(
            { parent_node->get_dma_scheduling_delay_cycle(),
              parent_node->get_complete_execution_cycle() });
      }
    }
  }
  // Compute the start and end cycles of merged all DMA intervals.
  int dma_merge_interval_start = std::numeric_limits<int>::max();
  int dma_merge_interval_end = std::numeric_limits<int>::min();
  for (auto& dma : dma_intervals) {
    if (dma.first < dma_merge_interval_start)
      dma_merge_interval_start = dma.first;
    if (dma.second > dma_merge_interval_end)
      dma_merge_interval_end = dma.second;
  }
  if (dma_merge_interval_end < sample->start_cycle) {
    // DMA finishes before the loop starts. There is no overlap.
    return;
  }
  sample->start_cycle = dma_merge_interval_end;
  sample->elapsed_cycle = sample->end_cycle - sample->start_cycle;
}

std::string repeatString(const std::string& str, int n) {
  std::stringstream ss;
  for (int i = 0; i < n; i++) {
    ss << str;
  }
  return ss.str();
}

void LoopSampling::printSampleTree(SampledLoop* node, int level) {
  // Sample tree printing format:
  //
  //  Root node, [sample info]
  //  +-loop0-sample0, [sample info]
  //  | +-loop1-sample0, [sample info]
  //  | | +-loop2-sample0, [sample info]
  //  | | +-loop2-sample1, [sample info]
  //  | +-loop1-sample1, [sample info]
  //  | | +-loop2-sample2, [sample info]
  //  | | +-loop2-sample3, [sample info]
  std::cout << repeatString("| ", level - 1);
  if (level > 0)
    std::cout << "+-";
  std::cout << node->str() << "\n";
  for (auto& child : node->children) {
    printSampleTree(child, level + 1);
  }
}

void LoopSampling::printSampleTree() {
  std::cout << "-------- Sample tree --------\n";
  printSampleTree(root, 0);
  std::cout << "-------- Sample tree --------\n";
}
