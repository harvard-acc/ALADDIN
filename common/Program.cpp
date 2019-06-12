#include "Program.h"

#include "DDDG.h"
#include "SourceEntity.h"

using namespace SrcTypes;

void Program::addEdge(unsigned int from, unsigned int to, uint8_t parid) {
  if (from != to) {
    add_edge(nodes.at(from)->get_vertex(), nodes.at(to)->get_vertex(),
             EdgeProperty(parid), graph);
  }
}

ExecNode* Program::insertNode(unsigned node_id, uint8_t microop) {
  nodes[node_id] = new ExecNode(node_id, microop);
  Vertex v = add_vertex(VertexProperty(node_id), graph);
  nodes.at(node_id)->set_vertex(v);
  assert(get(boost::vertex_node_id, graph, v) == node_id);
  return nodes.at(node_id);
}

void Program::clearExecNodes() {
  for (auto& node_pair : nodes)
    delete node_pair.second;
  nodes.clear();
}

void Program::clear() {
  clearExecNodes();
  graph.clear();
  call_arg_map.clear();
  loop_bounds.clear();
}

ExecNode* Program::getNextNode(unsigned node_id) const {
  auto it = nodes.find(node_id);
  assert(it != nodes.end());
  auto next_it = std::next(it);
  if (next_it == nodes.end())
    return nullptr;
  return next_it->second;
}

UniqueLabel Program::getUniqueLabel(ExecNode* node) const {
  // We'll only find a label if the labelmap is present in the dynamic trace,
  // but if the configuration file doesn't use labels (it's an older config
  // file), we have to fallback on using line numbers.
  auto range = labelmap.equal_range(node->get_line_num());
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second.get_function() != node->get_static_function())
      continue;
    return it->second;
  }
  return UniqueLabel();
}

std::list<cnode_pair_t> Program::findLoopBoundaries(
    const UniqueLabel* static_label, const DynamicLabel* dynamic_label) const {
  // Check which label we want to use. Only one label must be valid.
  bool static_mode = true;
  if (static_label && !dynamic_label) {
    static_mode = true;
  } else if (!static_label && dynamic_label) {
    static_mode = false;
  } else {
    std::cerr << "Either the static unique label or the dynamic label should "
                 "be valid.\n";
    exit(1);
  }
  std::list<cnode_pair_t> loop_boundaries;
  bool is_loop_executing = false;
  unsigned current_loop_depth = (unsigned)-1;

  const ExecNode* loop_start;

  // The loop boundaries provided by the accelerator are in a linear list with
  // no structure. We need to identify the start and end of each unrolled loop
  // section and add the corresponding pair of nodes to loop_boundaries.
  //
  // The last loop bound node is one past the last node, so we'll ignore it.
  for (auto it = loop_bounds.begin(); it != --loop_bounds.end(); ++it) {
    const DynLoopBound& loop_bound = *it;
    const ExecNode* node = nodes.at(loop_bound.node_id);
    const Vertex vertex = node->get_vertex();
    // Check if the loop information matches.
    bool is_same_loop = false;
    if (static_mode) {
      is_same_loop =
          node->get_line_num() == static_label->get_line_number() &&
          node->get_static_function() == static_label->get_function();
    } else {
      is_same_loop = node->get_line_num() == dynamic_label->get_line_number() &&
                     node->get_dynamic_function() ==
                         *(dynamic_label->get_dynamic_function());
    }
    if (boost::degree(vertex, graph) > 0 && is_same_loop) {
      if (!is_loop_executing) {
        if (loop_bound.target_loop_depth > node->get_loop_depth()) {
          is_loop_executing = true;
          loop_start = node;
          current_loop_depth = loop_bound.target_loop_depth;
        }
      } else {
        if (loop_bound.target_loop_depth == current_loop_depth) {
          // We're repeating an iteration of the same loop body.
          loop_boundaries.push_back(std::make_pair(loop_start, node));
          loop_start = node;
        } else if (loop_bound.target_loop_depth < current_loop_depth) {
          // We've left the loop.
          loop_boundaries.push_back(std::make_pair(loop_start, node));
          is_loop_executing = false;
          loop_start = NULL;
        } else if (loop_bound.target_loop_depth > current_loop_depth) {
          // We've entered an inner loop nest. We're not interested in the
          // inner loop nest.
          continue;
        }
      }
    }
  }
  return loop_boundaries;
}

std::list<cnode_pair_t> Program::findLoopBoundaries(
    const UniqueLabel& loop_label) const {
  return findLoopBoundaries(&loop_label, nullptr);
}

std::list<cnode_pair_t> Program::findLoopBoundaries(
    const DynamicLabel& loop_label) const {
  return findLoopBoundaries(nullptr, &loop_label);
}

std::list<cnode_pair_t> Program::findFunctionBoundaries(Function* func) const {
  std::list<cnode_pair_t> boundaries;
  if (!func)
    return boundaries;

  cnode_pair_t current_bound = std::make_pair(nullptr, nullptr);
  for (auto& node_id_pair : nodes) {
    const ExecNode* node = node_id_pair.second;
    if (node->is_call_op()) {
      const ExecNode* next_node = getNextNode(node->get_node_id());
      if (next_node->get_static_function() == func)
        current_bound.first = node;
    } else if (node->is_ret_op() && node->get_static_function() == func) {
      current_bound.second = node;
      if (current_bound.first == nullptr) {
        // This must have been a top level function, so just mark the very
        // first node as the start.
        current_bound.first = nodes.begin()->second;
      }
      boundaries.push_back(current_bound);

      // Reset to the default value.
      current_bound.first = nullptr;
      current_bound.second = nullptr;
    }
  }
  return boundaries;
}

std::vector<unsigned> Program::getConnectedNodes(unsigned int node_id) const {
  std::vector<unsigned> parentNodes = getParentNodes(node_id);
  std::vector<unsigned> childNodes = getChildNodes(node_id);
  parentNodes.insert(parentNodes.end(), childNodes.begin(), childNodes.end());
  return parentNodes;
}

std::vector<unsigned> Program::getParentNodes(unsigned int node_id) const {
  in_edge_iter in_edge_it, in_edge_end;
  ExecNode* node = nodes.at(node_id);
  Vertex vertex = node->get_vertex();

  std::vector<unsigned> connectedNodes;
  for (boost::tie(in_edge_it, in_edge_end) = in_edges(vertex, graph);
       in_edge_it != in_edge_end;
       ++in_edge_it) {
    Edge edge = *in_edge_it;
    Vertex source_vertex = source(edge, graph);
    connectedNodes.push_back(atVertex(source_vertex));
  }
  return connectedNodes;
}

std::vector<unsigned> Program::getChildNodes(unsigned int node_id) const {
  out_edge_iter out_edge_it, out_edge_end;
  ExecNode* node = nodes.at(node_id);
  Vertex vertex = node->get_vertex();

  std::vector<unsigned> connectedNodes;
  for (boost::tie(out_edge_it, out_edge_end) = out_edges(vertex, graph);
       out_edge_it != out_edge_end;
       ++out_edge_it) {
    Edge edge = *out_edge_it;
    Vertex target_vertex = target(edge, graph);
    connectedNodes.push_back(atVertex(target_vertex));
  }
  return connectedNodes;
}

int Program::getEdgeWeight(unsigned node_id_0, unsigned node_id_1) const {
  auto edge_to_parid = get(boost::edge_name, graph);
  ExecNode* node_0 = nodes.at(node_id_0);
  ExecNode* node_1 = nodes.at(node_id_1);
  auto edge_pair = edge(node_0->get_vertex(), node_1->get_vertex(), graph);
  if (!edge_pair.second)
    return -1;
  return edge_to_parid[edge_pair.first];
}

int Program::shortestDistanceBetweenNodes(unsigned from, unsigned to) const {
  std::list<std::pair<unsigned int, unsigned int>> queue;
  queue.push_back({ from, 0 });
  while (queue.size() != 0) {
    unsigned int curr_node = queue.front().first;
    unsigned int curr_dist = queue.front().second;
    out_edge_iter out_edge_it, out_edge_end;
    for (boost::tie(out_edge_it, out_edge_end) =
             out_edges(nodes.at(curr_node)->get_vertex(), graph);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      if (get(boost::edge_name, graph, *out_edge_it) != CONTROL_EDGE) {
        unsigned child_id = atVertex(target(*out_edge_it, graph));
        if (child_id == to)
          return curr_dist + 1;
        queue.push_back({ child_id, curr_dist + 1 });
      }
    }
    queue.pop_front();
  }
  return -1;
}

void Program::updateSamplingWithLabelInfo() {
  // Because we store the dynamic labels for the sampled loops, to update the
  // line numbers, here we need to find all that match the static label we get
  // from the label map. Same for the inlined loops below.
  for (auto it = labelmap.begin(); it != labelmap.end(); ++it) {
    const UniqueLabel& label_with_num = it->second;
    std::vector<std::pair<DynamicLabel, float>> to_add_sampled_loops;
    auto sample_it = sampled_loops.begin();
    while (sample_it != sampled_loops.end()) {
      auto sample_label = sample_it->first;
      if (sample_label.get_dynamic_function()->get_function() ==
              label_with_num.get_function() &&
          sample_label.get_label() == label_with_num.get_label()) {
        float factor = sample_it->second;
        DynamicLabel dyn_label = sample_label;
        dyn_label.set_line_number(label_with_num.get_line_number());
        to_add_sampled_loops.push_back(std::make_pair(dyn_label, factor));
        sample_it = sampled_loops.erase(sample_it);
      } else {
        ++sample_it;
      }
    }
    for (auto& sample_it : to_add_sampled_loops)
      sampled_loops[sample_it.first] = sample_it.second;
  }

  for (auto it = inline_labelmap.begin(); it != inline_labelmap.end(); ++it) {
    const UniqueLabel& inlined_label = it->first;
    const UniqueLabel& orig_label = it->second;
    std::vector<std::pair<DynamicLabel, float>> to_add_sampled_loops;
    for (auto sample_it = sampled_loops.begin();
         sample_it != sampled_loops.end();
         ++sample_it) {
      auto sample_label = sample_it->first;
      if (sample_label.get_dynamic_function()->get_function() ==
              orig_label.get_function() &&
          sample_label.get_label() == orig_label.get_label()) {
        float factor = sample_it->second;
        DynamicLabel dyn_label = sample_label;
        dyn_label.set_line_number(inlined_label.get_line_number());
        to_add_sampled_loops.push_back(std::make_pair(dyn_label, factor));
        sample_it = sampled_loops.erase(sample_it++);
      }
    }
    for (auto& sample_it : to_add_sampled_loops)
      sampled_loops[sample_it.first] = sample_it.second;
  }
}

int Program::upsampleLoops() {
  int correction_cycles = 0;
  for (auto& sample_it : sampled_loops) {
    float factor = sample_it.second;
    std::list<cnode_pair_t> loop_bound_nodes =
        findLoopBoundaries(sample_it.first);
    for (auto bound_it : loop_bound_nodes) {
      const ExecNode* first = bound_it.first;
      const ExecNode* second = bound_it.second;
      // Check every node between this loop boundary to see if it has a DMA load
      // parent. This is due to that we don't make any subsequent operations
      // block on a DMA operation (as long as there's no memory dependency), so
      // that a loop afterwards can start executing before the DMA operation. As
      // a result, the loop latency would include the DMA latency, which is not
      // intended for sampling purpose. Here, as a heuristic approach, we will
      // use the DMA completion time as the start cycle for the loop, based on
      // the fact that DMA operations normally have long latencies and we tend
      // to use the DMA data right away as we enter the loop.
      std::list<std::pair<int, int>> dma_intervals;
      for (unsigned int node_id = first->get_node_id() + 1;
           node_id < second->get_node_id();
           node_id++) {
           ExecNode* node = nodes.at(node_id);
           in_edge_iter in_edge_it, in_edge_end;
           for (boost::tie(in_edge_it, in_edge_end) =
                    in_edges(node->get_vertex(), graph);
                in_edge_it != in_edge_end;
                ++in_edge_it) {
             Vertex parent_vertex = source(*in_edge_it, graph);
             ExecNode* parent_node = nodeAtVertex(parent_vertex);
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
      for (auto& dma_it : dma_intervals) {
        if (dma_it.first < dma_merge_interval_start)
          dma_merge_interval_start = dma_it.first;
        if (dma_it.second > dma_merge_interval_end)
          dma_merge_interval_end = dma_it.second;
      }
      // Compute the DMA latency that is included in the loop latency.
      int loop_start_cycle = first->get_complete_execution_cycle();
      int loop_end_cycle = second->get_complete_execution_cycle();
      int dma_latency;
      if (dma_merge_interval_end < loop_start_cycle) {
        // DMA finishes before the loop starts. There is no overlap.
        dma_latency = 0;
      } else {
        if (dma_merge_interval_start < loop_start_cycle)
          dma_merge_interval_start = loop_start_cycle;
        dma_latency = dma_merge_interval_end - dma_merge_interval_start;
      }
      int latency = loop_end_cycle - loop_start_cycle - dma_latency;
      correction_cycles += latency * (factor - 1);
    }
  }
  return correction_cycles;
}
