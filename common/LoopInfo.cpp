#include <limits>

#include "LoopInfo.h"
#include "Program.h"

void LoopInfo::buildLoopTree() {
  insertRootNode();
  for (auto& line_label : program->labelmap) {
    std::list<cnode_pair_t> loop_bound_nodes =
        program->findLoopBoundaries(line_label.second);
    for (auto& bound : loop_bound_nodes) {
      const ExecNode* start_node = bound.first;
      const ExecNode* end_node = bound.second;
      const SrcTypes::UniqueLabel* label = &line_label.second;
      LoopIteration* loop = new LoopIteration(label, start_node, end_node);
      // Check if this loop iteration belongs to a sampled loop. If so, set its
      // sampling factor and mark it sampled.
      for (auto sample_it = loop_sampling_factors.begin();
           sample_it != loop_sampling_factors.end();
           ++sample_it) {
        const SrcTypes::DynamicLabel& sampled_label = sample_it->first;
        float factor = sample_it->second;
        bool is_sampled_loop =
            start_node->get_line_num() == sampled_label.get_line_number() &&
            start_node->get_dynamic_function() ==
                *(sampled_label.get_dynamic_function());
        if (is_sampled_loop) {
          loop->factor = factor;
          loop->sampled = true;
        }
      }
      loop_iters.push_back(loop);
      insertLoop(root, loop);
    }
  }
}

bool LoopInfo::insertLoop(LoopIteration* node, LoopIteration* loop) {
  if (node->contains(loop)) {
    // The loop should be inserted to the subtree from this node. We first
    // check if the node is a leaf node and directly insert the loop if so.
    // Otherwise, we look at every child of this node and see if the loop
    // will become a new parent of it (i.e., the loop will be inserted between
    // the node and any of its children). If not, we try to insert the loop to
    // the subtree from that child.
    if (node->children.size() == 0) {
      node->children.push_back(loop);
      loop->parent = node;
      return true;
    }
    bool inserted = false;
    auto child_it = node->children.begin();
    while (child_it != node->children.end()) {
      if (loop->contains(*child_it)) {
        loop->parent = node;
        loop->children.push_back(*child_it);
        (*child_it)->parent = loop;
        child_it = node->children.erase(child_it);
        if (std::find(node->children.begin(), node->children.end(), loop) ==
            node->children.end()) {
          child_it = node->children.insert(child_it, loop);
          ++child_it;
        }
        inserted = true;
      } else {
        inserted |= insertLoop(*child_it, loop);
        ++child_it;
      }
    }
    if (!inserted) {
      // The loop is neither a child nor a parent of any children, thus it's a
      // new sibling.
      node->children.push_back(loop);
      loop->parent = node;
    }
    return true;
  } else {
    // The loop can't be inserted into the path downward from the node.
    return false;
  }
}

std::string repeatString(const std::string& str, int n) {
  std::stringstream ss;
  for (int i = 0; i < n; i++) {
    ss << str;
  }
  return ss.str();
}

void LoopInfo::printLoopTree(LoopIteration* node, int level) {
  // Loop tree printing format:
  //
  //  Root node, [loop info]
  //  +-loop0-iter0, [loop info]
  //  | +-loop1-iter0, [loop info]
  //  | | +-loop2-iter0, [loop info]
  //  | | +-loop2-iter1, [loop info]
  //  | +-loop1-iter1, [loop info]
  //  | | +-loop2-iter2, [loop info]
  //  | | +-loop2-iter3, [loop info]
  //  ...
  std::cout << repeatString("| ", level - 1);
  if (level > 0)
    std::cout << "+-";
  std::cout << node->str() << "\n";
  for (auto& child : node->children) {
    printLoopTree(child, level + 1);
  }
}

void LoopInfo::printLoopTree() {
  std::cout << "-------- Loop tree --------\n";
  printLoopTree(root, 0);
  std::cout << "-------- Loop tree --------\n";
}

void LoopInfo::updateSamplingWithLabelInfo() {
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

void LoopInfo::upsampleLoop(LoopIteration* node, int correction_cycle) {
  if (node->isRootNode())
    node->elapsed_cycle += correction_cycle;
  else
    upsampleLoop(node->parent, correction_cycle * node->factor);
}

int LoopInfo::upsampleLoops() {
  // Set the default elapsed time and do DMA correction for every sampled loop
  // iteration.
  for (auto& loop : loop_iters) {
    if (loop->sampled) {
      loop->elapsed_cycle = loop->end_node->get_complete_execution_cycle() -
                            loop->start_node->get_complete_execution_cycle();
      sampleDmaCorrection(loop);
    }
  }

  // Upsample every sampled loop and propagate the sample execution upwards to
  // update the elapsed cycle of the root node.
  for (auto& loop : loop_iters) {
    if (loop->sampled && loop->parent && !loop->upsampled) {
      int correction_cycle = loop->elapsed_cycle * (loop->factor - 1);
      if (loop->pipelined) {
        // Different from an un-pipelined loop, we need special handling to
        // upsample a pipelined loop, as pipelined iterations have overlapped
        // intervals. To estimate the correction cycle, we will calculate the
        // average termination interval latency between two ajacent iterations.
        assert(loop->children.size() == 0 &&
               "Inner loops of a pipelined loop should be flattened!");
        // Find all siblings of this pipelined iteration and mark them upsampled
        // so that we won't upsample them twice.
        std::vector<LoopIteration*> pipelined_iters;
        for (auto& child : loop->parent->children) {
          if (*child->label == *loop->label) {
            pipelined_iters.push_back(child);
            child->upsampled = true;
          }
        }
        int total_termination_interval_cycle = 0;
        for (size_t i = 0; i < pipelined_iters.size(); i++) {
          if (i + 1 < pipelined_iters.size()) {
            total_termination_interval_cycle +=
                pipelined_iters[i + 1]
                    ->end_node->get_complete_execution_cycle() -
                pipelined_iters[i]->end_node->get_complete_execution_cycle();
          }
        }
        int avg_termination_interval_cycle =
            pipelined_iters.size() > 1 ? total_termination_interval_cycle /
                                             (pipelined_iters.size() - 1)
                                       : 0;
        correction_cycle = avg_termination_interval_cycle *
                           pipelined_iters.size() * (loop->factor - 1);
      }
      upsampleLoop(loop->parent, correction_cycle);
      loop->upsampled = true;
    }
  }
  return root->elapsed_cycle;
}

void LoopInfo::sampleDmaCorrection(LoopIteration* sample) {
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
  for (unsigned int node_id = sample->start_node->get_node_id() + 1;
       node_id < sample->end_node->get_node_id();
       node_id++) {
    ExecNode* node = program->nodes.at(node_id);
    in_edge_iter in_edge_it, in_edge_end;
    for (boost::tie(in_edge_it, in_edge_end) =
             in_edges(node->get_vertex(), program->graph);
         in_edge_it != in_edge_end;
         ++in_edge_it) {
      Vertex parent_vertex = source(*in_edge_it, program->graph);
      ExecNode* parent_node = program->nodeAtVertex(parent_vertex);
      if (parent_node->is_host_load()) {
        if (parent_node->is_dma_load()) {
          dma_intervals.push_back(
              { parent_node->get_dma_scheduling_delay_cycle(),
                parent_node->get_complete_execution_cycle() });
        } else {
          dma_intervals.push_back(
              { parent_node->get_start_execution_cycle(),
                parent_node->get_complete_execution_cycle() });
        }
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
  int sample_start_cycle = sample->start_node->get_complete_execution_cycle();
  int sample_end_cycle = sample->end_node->get_complete_execution_cycle();
  if (dma_merge_interval_end < sample_start_cycle) {
    // DMA finishes before the loop starts. There is no overlap.
    return;
  }
  sample->elapsed_cycle = sample_end_cycle - dma_merge_interval_end;
}
