#include <sstream>
#include <utility>

#include "opcode_func.h"
#include "BaseDatapath.h"
#include "ExecNode.h"

#include "DatabaseDeps.h"

using namespace SrcTypes;

std::ostream& operator<<(std::ostream& os, const LoopBoundDescriptor& obj) {
  os << "LoopBoundDescriptor(microop = " << obj.microop
     << ", loop_depth = " << obj.loop_depth
     << ", call depth = " << obj.call_depth
     << ", dyn_invocations = " << obj.dyn_invocations
     << ")";

  return os;
}

BaseDatapath::BaseDatapath(std::string& bench,
                           std::string& trace_file_name,
                           std::string& config_file)
    : benchName(bench), current_trace_off(0) {
  parse_config(benchName, config_file);

  use_db = false;
  if (!fileExists(trace_file_name)) {
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << " ERROR: Input Trace Not Found  " << std::endl;
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << "       Aladdin Ends..          " << std::endl;
    std::cerr << "-------------------------------" << std::endl;
    exit(1);
  }
  std::string file_name = benchName + "_summary";
  /* Remove the old file. */
  if (access(file_name.c_str(), F_OK) != -1 && remove(file_name.c_str()) != 0)
    perror("Failed to delete the old summary file");

  struct stat st;
  stat(trace_file_name.c_str(), &st);
  trace_size = st.st_size;
  trace_file = gzopen(trace_file_name.c_str(), "r");
}

BaseDatapath::~BaseDatapath() {
  gzclose(trace_file);
}

bool BaseDatapath::buildDddg() {
  DDDG* dddg;
  dddg = new DDDG(this, trace_file);
  /* Build initial DDDG. */
  current_trace_off = dddg->build_initial_dddg(current_trace_off, trace_size);
  if (labelmap.size() == 0) {
    labelmap = dddg->get_labelmap();
    updateUnrollingPipeliningWithLabelInfo(dddg->get_inline_labelmap());
  }
  delete dddg;

  if (current_trace_off == DDDG::END_OF_TRACE)
    return false;

  std::cout << "-------------------------------" << std::endl;
  std::cout << "    Initializing BaseDatapath      " << std::endl;
  std::cout << "-------------------------------" << std::endl;
  numTotalNodes = exec_nodes.size();
  beginNodeId = exec_nodes.begin()->first;
  endNodeId = (--exec_nodes.end())->first + 1;

  vertexToName = get(boost::vertex_node_id, graph_);

  num_cycles = 0;
  return true;
}

void BaseDatapath::updateUnrollingPipeliningWithLabelInfo(
    const inline_labelmap_t& inline_labelmap) {
  // The config file is parsed before the trace, so we don't have line number
  // information yet. After parsing the trace, update the unrolling and
  // pipelining config maps with line numbers.
  for (auto it = labelmap.begin(); it != labelmap.end(); ++it) {
    const UniqueLabel& label_with_num = it->second;
    UniqueLabel label_without_num(
        label_with_num.get_function(), label_with_num.get_label(), 0);

    auto unroll_it = unrolling_config.find(label_without_num);
    if (unroll_it != unrolling_config.end()) {
      unsigned factor = unroll_it->second;
      unrolling_config.erase(unroll_it);
      unrolling_config[label_with_num] = factor;
    }

    auto pipeline_it = pipeline_config.find(label_without_num);
    if (pipeline_it != pipeline_config.end()) {
      pipeline_config.erase(pipeline_it);
      pipeline_config.insert(label_with_num);
    }
  }

  // Update the unrolling/pipelining configurations for inlined labels.
  for (auto it = inline_labelmap.begin(); it != inline_labelmap.end(); ++it) {
    const UniqueLabel& inlined_label = it->first;
    const UniqueLabel& orig_label = it->second;

    auto unroll_it = unrolling_config.find(orig_label);
    if (unroll_it != unrolling_config.end())
      unrolling_config[inlined_label] = unroll_it->second;

    auto pipeline_it = pipeline_config.find(orig_label);
    if (pipeline_it != pipeline_config.end())
      pipeline_config.insert(inlined_label);
  }
}

void BaseDatapath::clearDatapath() {
  clearExecNodes();
  clearFunctionName();
  clearArrayBaseAddress();
  clearLoopBound();
  clearRegStats();
  graph_.clear();
  registers.clear();
  call_argument_map.clear();
}

void BaseDatapath::addDddgEdge(unsigned int from,
                               unsigned int to,
                               uint8_t parid) {
  if (from != to) {
    add_edge(exec_nodes.at(from)->get_vertex(), exec_nodes.at(to)->get_vertex(),
             EdgeProperty(parid), graph_);
  }
}

ExecNode* BaseDatapath::insertNode(unsigned node_id, uint8_t microop) {
  exec_nodes[node_id] = new ExecNode(node_id, microop);
  Vertex v = add_vertex(VertexProperty(node_id), graph_);
  exec_nodes.at(node_id)->set_vertex(v);
  assert(get(boost::vertex_node_id, graph_, v) == node_id);
  return exec_nodes.at(node_id);
}

void BaseDatapath::addCallArgumentMapping(DynamicVariable& callee_reg_id,
                                          DynamicVariable& caller_reg_id) {
  call_argument_map[callee_reg_id] = caller_reg_id;
}

DynamicVariable BaseDatapath::getCallerRegID(DynamicVariable& reg_id) {
  auto it = call_argument_map.find(reg_id);
  while (it != call_argument_map.end()) {
    reg_id = it->second;
    it = call_argument_map.find(reg_id);
  }
  return reg_id;
}

MemoryAddrSources BaseDatapath::findMemoryAddrSources(
    ExecNode* current_node,
    SrcTypes::Function* current_func,
    EdgeNameMap& edge_to_parid) {
  in_edge_iter in_edge_it, in_edge_end;
  MemoryAddrSources sources;
  for (boost::tie(in_edge_it, in_edge_end) =
           in_edges(current_node->get_vertex(), graph_);
       in_edge_it != in_edge_end;
       ++in_edge_it) {
    if (edge_to_parid[*in_edge_it] == CONTROL_EDGE)
      continue;
    Vertex source_vertex = source(*in_edge_it, graph_);
    ExecNode* parent_node = getNodeFromVertex(source_vertex);
    if (parent_node->is_load_op()) {
      sources.add_noninductive(parent_node);
      continue;
    }
    if (parent_node->is_compute_op() &&
        parent_node->get_static_function() == current_func) {
      if (parent_node->get_microop() == LLVM_IR_IndexAdd) {
        sources.add_inductive(parent_node);
      } else if (!parent_node->is_inductive()) {
        sources.merge(findMemoryAddrSources(
            parent_node, current_func, edge_to_parid));
      }
    }
  }
  if (sources.empty() && current_node->is_compute_op() &&
      !current_node->is_inductive() &&
      current_node->get_static_function() == current_func)
    sources.add_noninductive(current_node);
  sources.sort_and_uniquify();
  return sources;
}

void BaseDatapath::memoryAmbiguation() {
  std::cout << "-------------------------------" << std::endl;
  std::cout << "      Memory Ambiguation       " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  std::vector<newEdge> to_add_edges;
  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);
  using gep_store_pair_t = std::pair<ExecNode*, ExecNode*>;
  std::unordered_map<DynamicInstruction, std::vector<gep_store_pair_t>>
      possible_dependent_stores;
  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (!node->is_memory_op() || !node->has_vertex())
      continue;
    in_edge_iter in_edge_it, in_edge_end;
    for (boost::tie(in_edge_it, in_edge_end) =
             in_edges(node->get_vertex(), graph_);
         in_edge_it != in_edge_end;
         ++in_edge_it) {
      Vertex parent_vertex = source(*in_edge_it, graph_);
      ExecNode* parent_node = getNodeFromVertex(parent_vertex);
      if (parent_node->get_microop() != LLVM_IR_GetElementPtr)
        continue;
      if (!parent_node->is_inductive()) {
        node->set_dynamic_mem_op(true);
        if (node->is_store_op()) {
          DynamicInstruction unique_id = node->get_dynamic_instruction();
          if (possible_dependent_stores.find(unique_id) ==
              possible_dependent_stores.end())
            possible_dependent_stores[unique_id] =
                std::vector<gep_store_pair_t>();

          possible_dependent_stores[unique_id].push_back(
              std::make_pair(parent_node, node));
        }
      }
    }
  }

  // For each dynamic instruction that may potentially produce dependent
  // stores, check if the ops differ in their addresses by only an induction
  // variable. If so, then they are still dynamic memory ops but do not need to
  // be serialized.
  std::vector<DynamicInstruction> stores_to_serialize;
  for (const auto& kv_pair : possible_dependent_stores) {
    const std::vector<gep_store_pair_t>& store_list = kv_pair.second;
    std::list<MemoryAddrSources> all_sources;
    for (const gep_store_pair_t& pair : store_list) {
      ExecNode* gep = pair.first;
      all_sources.push_back(findMemoryAddrSources(
          gep, gep->get_static_function(), edge_to_parid));
    }
    unsigned idx = 0;
    for (auto it = all_sources.begin(); it != --all_sources.end(); ++idx) {
      MemoryAddrSources& curr_source = *it;
      MemoryAddrSources& next_source = *(++it);
      if (!curr_source.is_independent_of(next_source, idx == 0)) {
        to_add_edges.push_back({ store_list[idx].second,
                                 store_list[idx + 1].second, MEMORY_EDGE });
      }
    }
  }

  updateGraphWithNewEdges(to_add_edges);
}

/*
 * Read: graph_
 * Modify: graph_
 */
void BaseDatapath::removePhiNodes() {
  std::cout << "-------------------------------" << std::endl;
  std::cout << "  Remove PHI and Convert Nodes " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);
  std::set<Edge> to_remove_edges;
  std::vector<newEdge> to_add_edges;
  std::unordered_set<ExecNode*> checked_phi_nodes;
  for (auto node_it = exec_nodes.rbegin(); node_it != exec_nodes.rend();
       node_it++) {
    ExecNode* node = node_it->second;
    if (!node->has_vertex() ||
        (node->get_microop() != LLVM_IR_PHI && !node->is_convert_op()) ||
        (checked_phi_nodes.find(node) != checked_phi_nodes.end()))
      continue;
    Vertex node_vertex = node->get_vertex();
    // find its children
    std::vector<std::pair<ExecNode*, int>> phi_child;
    out_edge_iter out_edge_it, out_edge_end;
    for (boost::tie(out_edge_it, out_edge_end) = out_edges(node_vertex, graph_);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      checked_phi_nodes.insert(node);
      to_remove_edges.insert(*out_edge_it);
      Vertex v = target(*out_edge_it, graph_);
      phi_child.push_back(
          std::make_pair(getNodeFromVertex(v), edge_to_parid[*out_edge_it]));
    }
    if (phi_child.size() == 0 || boost::in_degree(node_vertex, graph_) == 0)
      continue;
    if (node->get_microop() == LLVM_IR_PHI) {
      // find its first non-phi ancestor.
      // phi node can have multiple children, but it can only have one parent.
      assert(boost::in_degree(node_vertex, graph_) == 1);
      in_edge_iter in_edge_it = in_edges(node_vertex, graph_).first;
      Vertex parent_vertex = source(*in_edge_it, graph_);
      ExecNode* nonphi_ancestor = getNodeFromVertex(parent_vertex);
      // Search for the first non-phi ancestor of the current phi node.
      while (nonphi_ancestor->get_microop() == LLVM_IR_PHI) {
        checked_phi_nodes.insert(nonphi_ancestor);
        assert(nonphi_ancestor->has_vertex());
        Vertex parent_vertex = nonphi_ancestor->get_vertex();
        if (boost::in_degree(parent_vertex, graph_) == 0)
          break;
        to_remove_edges.insert(*in_edge_it);
        in_edge_it = in_edges(parent_vertex, graph_).first;
        parent_vertex = source(*in_edge_it, graph_);
        nonphi_ancestor = getNodeFromVertex(parent_vertex);
      }
      to_remove_edges.insert(*in_edge_it);
      if (nonphi_ancestor->get_microop() != LLVM_IR_PHI) {
        // Add dependence between the current phi node's children and its first
        // non-phi ancestor.
        for (auto child_it = phi_child.begin(), chil_E = phi_child.end();
             child_it != chil_E;
             ++child_it) {
          to_add_edges.push_back(
              { nonphi_ancestor, child_it->first, child_it->second });
        }
      }
    } else {
      // convert nodes
      assert(boost::in_degree(node_vertex, graph_) == 1);
      in_edge_iter in_edge_it = in_edges(node_vertex, graph_).first;
      Vertex parent_vertex = source(*in_edge_it, graph_);
      ExecNode* nonphi_ancestor = getNodeFromVertex(parent_vertex);
      while (nonphi_ancestor->is_convert_op()) {
        checked_phi_nodes.insert(nonphi_ancestor);
        Vertex parent_vertex = nonphi_ancestor->get_vertex();
        if (boost::in_degree(parent_vertex, graph_) == 0)
          break;
        to_remove_edges.insert(*in_edge_it);
        in_edge_it = in_edges(parent_vertex, graph_).first;
        parent_vertex = source(*in_edge_it, graph_);
        nonphi_ancestor = getNodeFromVertex(parent_vertex);
      }
      to_remove_edges.insert(*in_edge_it);
      if (!nonphi_ancestor->is_convert_op()) {
        for (auto child_it = phi_child.begin(), chil_E = phi_child.end();
             child_it != chil_E;
             ++child_it) {
          to_add_edges.push_back(
              { nonphi_ancestor, child_it->first, child_it->second });
        }
      }
    }
    std::vector<std::pair<ExecNode*, int>>().swap(phi_child);
  }

  updateGraphWithIsolatedEdges(to_remove_edges);
  updateGraphWithNewEdges(to_add_edges);
  cleanLeafNodes();
}
/*
 * Read: lineNum.gz, flattenConfig
 * Modify: graph_
 */
void BaseDatapath::loopFlatten() {
  if (!unrolling_config.size())
    return;
  std::cout << "-------------------------------" << std::endl;
  std::cout << "         Loop Flatten          " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  std::vector<unsigned> to_remove_nodes;
  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    auto config_it = getUnrollFactor(node);
    if (config_it == unrolling_config.end() || config_it->second != 0)
      continue;
    if (node->is_compute_op())
      node->set_microop(LLVM_IR_Move);
    else if (node->is_branch_op())
      to_remove_nodes.push_back(node->get_node_id());
  }
  updateGraphWithIsolatedNodes(to_remove_nodes);
  cleanLeafNodes();
}

void BaseDatapath::cleanLeafNodes() {
  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  /*track the number of children each node has*/
  std::map<unsigned, int> num_of_children;
  for (auto& node_it : exec_nodes)
    num_of_children[node_it.first] = 0;
  std::vector<unsigned> to_remove_nodes;

  std::vector<Vertex> topo_nodes;
  boost::topological_sort(graph_, std::back_inserter(topo_nodes));
  // bottom nodes first
  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi) {
    Vertex node_vertex = *vi;
    if (boost::degree(node_vertex, graph_) == 0)
      continue;
    unsigned node_id = vertexToName[node_vertex];
    ExecNode* node = exec_nodes.at(node_id);
    int node_microop = node->get_microop();
    if (num_of_children.at(node_id) == boost::out_degree(node_vertex, graph_) &&
        node_microop != LLVM_IR_SilentStore && node_microop != LLVM_IR_Store &&
        node_microop != LLVM_IR_Ret && !node->is_branch_op() &&
        !node->is_dma_store()) {
      to_remove_nodes.push_back(node_id);
      // iterate its parents
      in_edge_iter in_edge_it, in_edge_end;
      for (boost::tie(in_edge_it, in_edge_end) = in_edges(node_vertex, graph_);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        int parent_id = vertexToName[source(*in_edge_it, graph_)];
        num_of_children.at(parent_id)++;
      }
    } else if (node->is_branch_op()) {
      // iterate its parents
      in_edge_iter in_edge_it, in_edge_end;
      for (boost::tie(in_edge_it, in_edge_end) = in_edges(node_vertex, graph_);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        if (edge_to_parid[*in_edge_it] == CONTROL_EDGE) {
          int parent_id = vertexToName[source(*in_edge_it, graph_)];
          num_of_children.at(parent_id)++;
        }
      }
    }
  }
  updateGraphWithIsolatedNodes(to_remove_nodes);
}
/*
 * Read: graph_, instid
 */
void BaseDatapath::removeInductionDependence() {
  // set graph
  std::cout << "-------------------------------" << std::endl;
  std::cout << "  Remove Induction Dependence  " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);
  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    // Revert the inductive flag to false. This guarantees that no child node
    // of this node can mistakenly think its parent is inductive.
    node->set_inductive(false);
    if (!node->has_vertex() || node->is_memory_op())
      continue;
    Instruction* node_inst = node->get_static_inst();
    if (node_inst->is_inductive()) {
      node->set_inductive(true);
      if (node->is_int_add_op())
        node->set_microop(LLVM_IR_IndexAdd);
      else if (node->is_int_mul_op())
        node->set_microop(LLVM_IR_Shl);
    } else {
      /* If all the parents are inductive, then the current node is inductive.*/
      bool all_inductive = true;
      /* If one of the parent is inductive, and the operation is an integer mul,
       * do strength reduction that converts the mul/div to a shifter.*/
      bool any_inductive = false;
      in_edge_iter in_edge_it, in_edge_end;
      for (boost::tie(in_edge_it, in_edge_end) =
               in_edges(node->get_vertex(), graph_);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        if (edge_to_parid[*in_edge_it] == CONTROL_EDGE)
          continue;
        Vertex parent_vertex = source(*in_edge_it, graph_);
        ExecNode* parent_node = getNodeFromVertex(parent_vertex);
        if (!parent_node->is_inductive()) {
          all_inductive = false;
        } else {
          any_inductive = true;
        }
      }
      if (all_inductive) {
        node->set_inductive(true);
        if (node->is_int_add_op())
          node->set_microop(LLVM_IR_IndexAdd);
        else if (node->is_int_mul_op())
          node->set_microop(LLVM_IR_Shl);
      } else if (any_inductive) {
        if (node->is_int_mul_op())
          node->set_microop(LLVM_IR_Shl);
      }
    }
  }
}

// called in the end of the whole flow
void BaseDatapath::dumpStats() {
  rescheduleNodesWhenNeeded();
  updateRegStats();
  writePerCycleActivity();
#ifdef DEBUG
  dumpGraph(benchName);
  writeOtherStats();
#endif
}

void BaseDatapath::loopPipelining() {
  if (!pipelining) {
    std::cerr << "Global loop pipelining is not ON." << std::endl;
    return;
  }

  if (!unrolling_config.size()) {
    std::cerr << "Loop Unrolling is not defined. " << std::endl;
    std::cerr << "Loop pipelining is only applied to unrolled loops."
              << std::endl;
    return;
  }

  if (loopBound.size() <= 2)
    return;
  std::cout << "-------------------------------" << std::endl;
  std::cout << "         Loop Pipelining        " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  vertex_iter vi, vi_end;
  std::set<Edge> to_remove_edges;
  std::vector<newEdge> to_add_edges;

  // After loop unrolling, we define strict control dependences between basic
  // block, where all the instructions in the following basic block depend on
  // the prev branch instruction. To support loop pipelining, which allows the
  // next iteration starting without waiting until the prev iteration finish,
  // we move the control dependences between last branch node in the prev basic
  // block and instructions in the next basic block to first non isolated
  // instruction in the prev basic block and instructions in the next basic
  // block...

  // TODO: Leave first_non_isolated_node as storing node ids for now. Once I've
  // gotten everything else working, I'll come back to this.
  std::map<unsigned, unsigned> first_non_isolated_node;
  auto bound_it = loopBound.begin();
  auto node_it = exec_nodes.begin();
  ExecNode* curr_node = exec_nodes.at(bound_it->node_id);
  bound_it++;  // skip first region
  while (node_it != exec_nodes.end()) {
    assert(exec_nodes.at(bound_it->node_id)->is_branch_op());
    while (node_it != exec_nodes.end() && node_it->first < bound_it->node_id) {
      curr_node = node_it->second;
      if (!curr_node->has_vertex() ||
          boost::degree(curr_node->get_vertex(), graph_) == 0 ||
          curr_node->is_branch_op()) {
        ++node_it;
        continue;
      } else {
        first_non_isolated_node[bound_it->node_id] = curr_node->get_node_id();
        node_it = exec_nodes.find(bound_it->node_id);
        assert(node_it != exec_nodes.end());
        break;
      }
    }
    if (first_non_isolated_node.find(bound_it->node_id) ==
        first_non_isolated_node.end())
      first_non_isolated_node[bound_it->node_id] = bound_it->node_id;
    bound_it++;
    if (bound_it == loopBound.end() - 1)
      break;
  }

  ExecNode* prev_branch_n = nullptr;
  ExecNode* prev_first_n = nullptr;
  for (auto first_it = first_non_isolated_node.begin(),
            E = first_non_isolated_node.end();
       first_it != E;
       ++first_it) {
    ExecNode* br_node = exec_nodes.at(first_it->first);
    ExecNode* first_node = exec_nodes.at(first_it->second);
    bool found = false;
    auto unroll_it = getUnrollFactor(br_node);
    /* We only want to pipeline loop iterations that are from the same unrolled
     * loop. Here we first check the current basic block is part of an unrolled
     * loop iteration. */
    if (unroll_it != unrolling_config.end()) {
      // check whether the previous branch is the same loop or not
      if (prev_branch_n != nullptr) {
        if ((br_node->get_line_num() == prev_branch_n->get_line_num()) &&
            (br_node->get_static_function() ==
             prev_branch_n->get_static_function()) &&
            first_node->get_line_num() == prev_first_n->get_line_num()) {
          found = true;
        }
      }
    }
    /* We only pipeline matching loop iterations. */
    if (!found) {
      prev_branch_n = br_node;
      prev_first_n = first_node;
      continue;
    }
    // adding dependence between prev_first and first_id
    if (!doesEdgeExist(prev_first_n, first_node)) {
      to_add_edges.push_back({ prev_first_n, first_node, CONTROL_EDGE });
    }
    // adding dependence between first_id and prev_branch's children
    assert(prev_branch_n->has_vertex());
    out_edge_iter out_edge_it, out_edge_end;
    for (boost::tie(out_edge_it, out_edge_end) =
             out_edges(prev_branch_n->get_vertex(), graph_);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      Vertex child_vertex = target(*out_edge_it, graph_);
      ExecNode* child_node = getNodeFromVertex(child_vertex);
      if (*child_node < *first_node ||
          edge_to_parid[*out_edge_it] != CONTROL_EDGE)
        continue;
      if (!doesEdgeExist(first_node, child_node)) {
        to_add_edges.push_back({ first_node, child_node, 1 });
      }
    }
    // update first_id's parents, dependence become strict control dependence
    assert(first_node->has_vertex());
    in_edge_iter in_edge_it, in_edge_end;
    for (boost::tie(in_edge_it, in_edge_end) =
             in_edges(first_node->get_vertex(), graph_);
         in_edge_it != in_edge_end;
         ++in_edge_it) {
      Vertex parent_vertex = source(*in_edge_it, graph_);
      ExecNode* parent_node = getNodeFromVertex(parent_vertex);
      // unsigned parent_id = vertexToName[parent_vertex];
      if (parent_node->is_branch_op())
        continue;
      to_remove_edges.insert(*in_edge_it);
      to_add_edges.push_back({ parent_node, first_node, CONTROL_EDGE });
    }
    // remove control dependence between prev br node to its children
    assert(prev_branch_n->has_vertex());
    for (boost::tie(out_edge_it, out_edge_end) =
             out_edges(prev_branch_n->get_vertex(), graph_);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      if (exec_nodes.at(vertexToName[target(*out_edge_it, graph_)])->is_call_op())
        continue;
      if (edge_to_parid[*out_edge_it] != CONTROL_EDGE)
        continue;
      to_remove_edges.insert(*out_edge_it);
    }
    prev_branch_n = br_node;
    prev_first_n = first_node;
  }

  updateGraphWithNewEdges(to_add_edges);
  updateGraphWithIsolatedEdges(to_remove_edges);
  cleanLeafNodes();
}

void BaseDatapath::perLoopPipelining() {
  if (loopBound.size() <= 2 || pipeline_config.size() == 0)
    return;

  if (pipelining) {
    std::cerr
        << "Per loop pipelining cannot be used in conjunction with the "
           "legacy global pipelining! Please disable the global pipelining "
           "setting first.\n";
    return;
  }

  std::cout << "-------------------------------" << std::endl;
  std::cout << "      Per Loop Pipelining      " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);
  std::set<Edge> to_remove_edges;
  std::vector<newEdge> to_add_edges;

  // Strategy: for every loop I want to pipeline:
  //  1. Find every node that starts an iteration of this loop.
  //  2. Find the first non-isolated node (FNIN) from this iteration.
  //  3. For all control dependences going to instructions in an iteration and
  //     originating from the branch node that marked the boundary of the
  //     previous iteration:
  //     a. Change the source to the FNIN of the previous iteration.
  for (const UniqueLabel& loop : pipeline_config) {
    // Step 1.
    std::list<node_pair_t> current_loop_bounds = findLoopBoundaries(loop);

    // Step 2.
    std::map<unsigned, unsigned> first_non_isolated_nodes;
    for (auto bound_it = current_loop_bounds.begin(),
              end_it = current_loop_bounds.end();
         bound_it != end_it; ++bound_it) {
      unsigned curr_bound_node_id = bound_it->first->get_node_id();
      unsigned next_bound_node_id = bound_it->second->get_node_id();
      auto it = exec_nodes.find(curr_bound_node_id);
      // Find the FNIN after this boundary node (so, don't consider the
      // boundary node itself).
      ++it;
      unsigned node_id = it->first;
      bool is_isolated = true;
      while (node_id < next_bound_node_id) {
        ExecNode* next_node = it->second;
        is_isolated = (!next_node->has_vertex() ||
                       boost::degree(next_node->get_vertex(), graph_) == 0 ||
                       next_node->is_branch_op());
        if (!is_isolated) {
          // Use the next bound node as the key, not the current boundary node,
          // because this is exactly how we will change the control dependence
          // edge sources in the next step.
          first_non_isolated_nodes[next_bound_node_id] = node_id;
          break;
        } else {
          it++;
          node_id = it->first;
        }
      }
    }

    // Step 3.
    ExecNode* prev_branch_node = nullptr;
    ExecNode* prev_first_node = nullptr;
    for (auto first_it = first_non_isolated_nodes.begin(),
              end_it = first_non_isolated_nodes.end();
         first_it != end_it;
         ++first_it) {
      ExecNode* branch_node = exec_nodes.at(first_it->first);
      ExecNode* first_node = exec_nodes.at(first_it->second);
      if (!prev_branch_node && !prev_first_node) {
        prev_branch_node = branch_node;
        prev_first_node = first_node;
        continue;
      }

      if (!doesEdgeExist(prev_first_node, first_node)) {
        to_add_edges.push_back({ prev_first_node, first_node, CONTROL_EDGE });
      }
      // Identify all nodes in the body of the previous iteration that succeed
      // the FNIN of that iteration. If any of these nodes have a control
      // dependence on that iteration's entry branch node, change the source of
      // that edge to be the FNIN.
      //
      // This pass just adds that new edge. The original edge will be removed
      // later.
      assert(prev_branch_node->has_vertex());
      out_edge_iter out_edge_it, out_edge_end;
      for (boost::tie(out_edge_it, out_edge_end) =
               out_edges(prev_branch_node->get_vertex(), graph_);
           out_edge_it != out_edge_end;
           ++out_edge_it) {
        Vertex child_vertex = target(*out_edge_it, graph_);
        ExecNode* child_node = getNodeFromVertex(child_vertex);
        if (*child_node >= *first_node &&
            edge_to_parid[*out_edge_it] == CONTROL_EDGE) {
          // TODO: What is the meaning of EDGE = 1? It gets used in Load/Store
          // Buffer.
          if (!doesEdgeExist(first_node, child_node))
            to_add_edges.push_back({ first_node, child_node, 1 });
        }
      }

      // Pipelining causes the first non-isolated nodes to be the real
      // iteration bounds (rather than branch nodes), so any dependences of
      // FNINs must be converted to control edges.
      assert(first_node->has_vertex());
      in_edge_iter in_edge_it, in_edge_end;
      for (boost::tie(in_edge_it, in_edge_end) =
               in_edges(first_node->get_vertex(), graph_);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        Vertex parent_vertex = source(*in_edge_it, graph_);
        ExecNode* parent_node = getNodeFromVertex(parent_vertex);
        // TODO: Ignore branch nodes since they are typically loop bounds, but
        // what if we have control flow with a loop?
        if (!parent_node->is_branch_op()) {
          to_remove_edges.insert(*in_edge_it);
          to_add_edges.push_back({ parent_node, first_node, CONTROL_EDGE });
        }
      }

      // Remove all control dependences from the previous branch node to its
      // children (because these dependences have been moved to the previous
      // FNIN), except for call nodes (we don't optimize across function
      // boundaries).
      assert(prev_branch_node->has_vertex());
      for (boost::tie(out_edge_it, out_edge_end) =
               out_edges(prev_branch_node->get_vertex(), graph_);
           out_edge_it != out_edge_end;
           ++out_edge_it) {
        unsigned target_node_id = vertexToName[target(*out_edge_it, graph_)];
        if (exec_nodes.at(target_node_id)->is_call_op())
          continue;
        if (edge_to_parid[*out_edge_it] != CONTROL_EDGE)
          continue;
        to_remove_edges.insert(*out_edge_it);
      }
      prev_branch_node = branch_node;
      prev_first_node = first_node;
    }
  }

  updateGraphWithNewEdges(to_add_edges);
  updateGraphWithIsolatedEdges(to_remove_edges);
  cleanLeafNodes();
}

/*
 * Read: graph_, lineNum.gz, unrollingConfig
 * Modify: graph_
 * Write: loop_bound
 */
void BaseDatapath::loopUnrolling() {
  if (!unrolling_config.size()) {
    std::cerr << "No loop unrolling configs found." << std::endl;
    return;
  }
  std::cout << "-------------------------------" << std::endl;
  std::cout << "         Loop Unrolling        " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  std::stack<LoopBoundDescriptor> loop_nests;
  std::vector<unsigned> to_remove_nodes;
  std::vector<ExecNode*> nodes_between;
  std::vector<newEdge> to_add_edges;

  bool first = false;
  int iter_counts = 0;
  unsigned curr_call_depth = 0;
  ExecNode* prev_branch = nullptr;

  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (!node->has_vertex())
      continue;
    if (node->get_microop() == LLVM_IR_Ret) {
      curr_call_depth--;
      // Clear all loop nests from the stack in case we return from an inner
      // loop.
      while (!loop_nests.empty() &&
             loop_nests.top().call_depth > curr_call_depth) {
        loop_nests.pop();
      }
    }
    Vertex node_vertex = node->get_vertex();
    // We let all the branch nodes proceed to the unrolling handling no matter
    // whether they are isolated or not. Although most of the branch nodes are
    // connected anyway, one exception is unconditional branch that is not
    // dependent on any nodes. The is_branch_op() check can let the
    // unconditional branch proceed.
    if (boost::degree(node_vertex, graph_) == 0 && !node->is_branch_op())
      continue;
    if (ready_mode && node->is_dma_load())
      continue;
    unsigned node_id = node->get_node_id();
    ExecNode* next_node = getNextNode(node_id);
    if (!next_node)
      continue;
    DynLoopBound dyn_bound(node_id, next_node->get_loop_depth());
    if (!first) {
      // prev_branch should not be anything but a branch node.
      if (node->is_branch_op()) {
        first = true;
        loopBound.push_back(dyn_bound);
        prev_branch = node;
      } else {
        continue;
      }
    }
    assert(prev_branch != nullptr);
    // We should never add control edges to DMA nodes. They should be
    // constrained by memory dependences only.
    if (prev_branch != node && !node->is_dma_op()) {
      to_add_edges.push_back({ prev_branch, node, CONTROL_EDGE });
    }
    // If the current node is not a branch node, it will not be a boundary node.
    if (!node->is_branch_op()) {
      if (!node->is_dma_op())
        nodes_between.push_back(node);
    } else {
      // for the case that the first non-isolated node is also a call node;
      if (node->is_call_op() && loopBound.rbegin()->node_id != node_id) {
        loopBound.push_back(dyn_bound);
        curr_call_depth++;
        prev_branch = node;
      }
      auto unroll_it = getUnrollFactor(node);
      if (unroll_it == unrolling_config.end() || unroll_it->second == 0) {
        // not unrolling branch
        if (!node->is_call_op() && !node->is_dma_op()) {
          nodes_between.push_back(node);
          continue;
        }
        if (!doesEdgeExist(prev_branch, node) && !node->is_dma_op()) {
          // Enforce dependences between branch nodes, including call nodes
          to_add_edges.push_back({ prev_branch, node, CONTROL_EDGE });
        }
        for (auto prev_node_it = nodes_between.begin(), E = nodes_between.end();
             prev_node_it != E;
             prev_node_it++) {
          if (!doesEdgeExist(*prev_node_it, node)) {
            to_add_edges.push_back({ *prev_node_it, node, CONTROL_EDGE });
          }
        }
        nodes_between.clear();
        nodes_between.push_back(node);
        prev_branch = node;
      } else {  // unrolling the branch
        // This is the loop descriptor we're trying to find in the loop nest
        // stack.
        LoopBoundDescriptor loop_id(
            next_node->get_basic_block(), node->get_microop(),
            next_node->get_loop_depth(), curr_call_depth);
        // Once we've found the loop descriptor, use this pointer to the top of
        // the stack to update the invocations count.
        LoopBoundDescriptor* curr_loop = nullptr;

        /* Four possibilities with this loop:
         * 1. We're entering a new loop nest (possibly due to a Call).
         * 2. We're exiting the current innermost loop.
         * 3. We're repeating an inner loop.
         * 4. We're jumping to a different loop entirely due to a goto.
         */
        if (loop_nests.empty() ||
            loop_nests.top().loop_depth < loop_id.loop_depth ||
            loop_nests.top().call_depth < curr_call_depth) {
          // If our loop depth is greater than the top of the stack, add a new
          // loop nest.
          loop_nests.push(loop_id);
          curr_loop = &loop_nests.top();
        } else if (loop_nests.top().exitBrIs(loop_id)) {
          curr_loop = &loop_nests.top();
          loop_nests.pop();
        } else if (loop_nests.top() == loop_id) {
          // We're repeating the same loop. Nothing to do.
          curr_loop = &loop_nests.top();
        } else {
          // We've jumped to a different loop entirely.
          while (!loop_nests.empty() &&
                 loop_nests.top() != loop_id &&
                 loop_nests.top().loop_depth >= loop_id.loop_depth) {
            loop_nests.pop();
          }

          // Did not find anything matching this loop boundary node. Add a new
          // one. This happens if the branch was a goto from a disjoint loop, but it
          // can also happen if we don't have any loop depth information (aka
          // all loops have depth 0).
          if (loop_nests.empty() || loop_nests.top() != loop_id) {
            loop_nests.push(loop_id);
          }
          curr_loop = &loop_nests.top();
        }

        // Counting number of loop iterations.
        int unroll_factor = unroll_it->second;
        curr_loop->dyn_invocations++;
        if (curr_loop->dyn_invocations % unroll_factor == 0) {
          if (loopBound.rbegin()->node_id != node_id) {
            loopBound.push_back(dyn_bound);
          }
          iter_counts++;
          for (auto prev_node_it = nodes_between.begin(),
                    E = nodes_between.end();
               prev_node_it != E;
               prev_node_it++) {
            if (!doesEdgeExist(*prev_node_it, node)) {
              to_add_edges.push_back({ *prev_node_it, node, CONTROL_EDGE });
            }
          }
          nodes_between.clear();
          nodes_between.push_back(node);
          prev_branch = node;
        } else {
          to_remove_nodes.push_back(node_id);
        }
      }
    }
  }
  loopBound.push_back(DynLoopBound(numTotalNodes, 0));

  if (iter_counts == 0 && unrolling_config.size() != 0) {
    std::cerr << "-------------------------------\n"
              << "Loop Unrolling was NOT applied.\n"
              << "Either loop labels or line numbers are incorrect, or the\n"
              << "loop unrolling factor is larger than the loop trip count.\n"
              << "-------------------------------" << std::endl;
  }
  updateGraphWithNewEdges(to_add_edges);
  updateGraphWithIsolatedNodes(to_remove_nodes);
  cleanLeafNodes();
}

void BaseDatapath::fuseRegLoadStores() {
  std::cout << "-------------------------------" << std::endl;
  std::cout << "  Fuse register loads/stores   " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  // A value stored in a register does not need to take one cycle for a "Load"
  // before it can be used. Similarly, a value being written to a register does
  // not require an entire cycle, as long as whatever is producing the value
  // completes before the required setup time of the clock edge. But LLVM will
  // emit load and store IR instructions for all arrays as it does not know
  // that some arrays are not actually SRAM structures. For completely partitioned
  // arrays, convert the memory dependence edge between loads, the consuming
  // node, and the store into a special register dependence edge. This tells
  // the scheduler to schedule the two nodes on the same cycle.

  std::vector<newEdge> to_add_edges;
  std::set<Edge> to_remove_edges;
  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  for (auto& node_pair : exec_nodes) {
    ExecNode* node = node_pair.second;
    if (!node->is_load_op() && !node->is_store_op())
      continue;
    auto part_it = partition_config.find(node->get_array_label());
    if (part_it == partition_config.end() ||
        part_it->second.partition_type != complete)
      continue;

    if (node->is_load_op()) {
      Vertex load_vertex = node->get_vertex();
      out_edge_iter out_edge_it, out_edge_end;
      for (boost::tie(out_edge_it, out_edge_end) = out_edges(load_vertex, graph_);
           out_edge_it != out_edge_end;
           ++out_edge_it) {
        if (edge_to_parid[*out_edge_it] == CONTROL_EDGE)
          continue;

        Vertex target_vertex = target(*out_edge_it, graph_);
        ExecNode* target_node = getNodeFromVertex(target_vertex);
        // A register load cannot be fused with another load operation.
        if (target_node->is_load_op())
          continue;

        to_remove_edges.insert(*out_edge_it);
        to_add_edges.push_back({ node, target_node, REGISTER_EDGE });
      }
    } else if (node->is_store_op()) {
      Vertex store_vertex = node->get_vertex();
      in_edge_iter in_edge_it, in_edge_end;
      for (boost::tie(in_edge_it, in_edge_end) = in_edges(store_vertex, graph_);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        if (edge_to_parid[*in_edge_it] == CONTROL_EDGE)
          continue;

        Vertex source_vertex = source(*in_edge_it, graph_);
        ExecNode* source_node = getNodeFromVertex(source_vertex);
        // A register store cannot be fused with another store operation.
        if (source_node->is_store_op())
          continue;

        to_remove_edges.insert(*in_edge_it);
        to_add_edges.push_back({ source_node, node, REGISTER_EDGE });
      }
    }
  }

  updateGraphWithIsolatedEdges(to_remove_edges);
  updateGraphWithNewEdges(to_add_edges);
  cleanLeafNodes();
}

void BaseDatapath::findBranchChain(ExecNode* root,
                                   std::list<ExecNode*>& branch_chain,
                                   std::set<Edge>& to_remove_edges) {

  if (boost::out_degree(root->get_vertex(), graph_) != 1)
    return;
  out_edge_iter out_edge_it, out_edge_end;
  for (boost::tie(out_edge_it, out_edge_end) =
           out_edges(root->get_vertex(), graph_);
       out_edge_it != out_edge_end;
       ++out_edge_it) {
    Vertex target_vertex = target(*out_edge_it, graph_);
    ExecNode* target_node = getNodeFromVertex(target_vertex);
    if (target_node->is_branch_op() || target_node->is_call_op()) {
      branch_chain.push_back(target_node);
      to_remove_edges.insert(*out_edge_it);
      findBranchChain(target_node, branch_chain, to_remove_edges);
    }
  }
}

void BaseDatapath::fuseConsecutiveBranches() {
  std::cout << "-------------------------------" << std::endl;
  std::cout << "   Fuse consecutive branches   " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  // Aladdin enforces control dependences for all branch nodes. But sometimes
  // we will see multiple branch nodes consecutively, particularly in the case
  // of multiply nested loops and preheader basic blocks. This can introduce a
  // significant amount of additional latency. This optimization assumes that
  // an FSM can always determine in a single cycle which loop nest to execute
  // next and fuses consecutive branch nodes together.

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);
  std::set<Edge> to_remove_edges;
  std::vector<newEdge> to_add_edges;

  std::list<Vertex> topo_nodes;
  boost::topological_sort(graph_, std::front_inserter(topo_nodes));

  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi) {
    Vertex vertex = *vi;
    ExecNode* node = getNodeFromVertex(vertex);
    if (!(node->is_branch_op() || node->is_call_op()))
      continue;
    if (boost::out_degree(vertex, graph_) != 1)
      continue;

    std::list<ExecNode*> branch_chain{ node };
    findBranchChain(node, branch_chain, to_remove_edges);
    if (branch_chain.size() > 1) {
      for (auto it = branch_chain.begin(); it != --branch_chain.end();)
        to_add_edges.push_back({ *it, *(++it), FUSED_BRANCH_EDGE });
    }
  }

  updateGraphWithIsolatedEdges(to_remove_edges);
  updateGraphWithNewEdges(to_add_edges);
  cleanLeafNodes();
}

/*
 * Read: loop_bound, flattenConfig, graph, actualAddress
 * Modify: graph_
 */
void BaseDatapath::removeSharedLoads() {

  if (!unrolling_config.size() && loopBound.size() <= 2)
    return;
  std::cout << "-------------------------------" << std::endl;
  std::cout << "          Load Buffer          " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  vertex_iter vi, vi_end;

  std::set<Edge> to_remove_edges;
  std::vector<newEdge> to_add_edges;

  int shared_loads = 0;
  auto bound_it = loopBound.begin();

  auto node_it = exec_nodes.begin();
  while (node_it != exec_nodes.end()) {
    std::unordered_map<unsigned, ExecNode*> address_loaded;
    while (node_it->first < bound_it->node_id && node_it != exec_nodes.end()) {
      ExecNode* node = node_it->second;
      if (!node->has_vertex() ||
          boost::degree(node->get_vertex(), graph_) == 0 ||
          !node->is_memory_op()) {
        node_it++;
        continue;
      }
      long long int node_address = node->get_mem_access()->vaddr;
      auto addr_it = address_loaded.find(node_address);
      if (node->is_store_op() && addr_it != address_loaded.end()) {
        address_loaded.erase(addr_it);
      } else if (node->is_load_op()) {
        if (addr_it == address_loaded.end()) {
          address_loaded[node_address] = node;
        } else {
          // check whether the current load is dynamic or not.
          if (node->is_dynamic_mem_op()) {
            ++node_it;
            continue;
          }
          shared_loads++;
          node->set_microop(LLVM_IR_Move);
          ExecNode* prev_load = addr_it->second;
          // iterate through its children
          Vertex load_node = node->get_vertex();
          out_edge_iter out_edge_it, out_edge_end;
          for (boost::tie(out_edge_it, out_edge_end) =
                   out_edges(load_node, graph_);
               out_edge_it != out_edge_end;
               ++out_edge_it) {
            Edge curr_edge = *out_edge_it;
            Vertex child_vertex = target(curr_edge, graph_);
            assert(prev_load->has_vertex());
            Vertex prev_load_vertex = prev_load->get_vertex();
            if (!doesEdgeExistVertex(prev_load_vertex, child_vertex)) {
              ExecNode* child_node = getNodeFromVertex(child_vertex);
              to_add_edges.push_back(
                  { prev_load, child_node, edge_to_parid[curr_edge] });
            }
            to_remove_edges.insert(*out_edge_it);
          }
          in_edge_iter in_edge_it, in_edge_end;
          for (boost::tie(in_edge_it, in_edge_end) =
                   in_edges(load_node, graph_);
               in_edge_it != in_edge_end;
               ++in_edge_it)
            to_remove_edges.insert(*in_edge_it);
        }
      }
      node_it++;
    }
    bound_it++;
    if (bound_it == loopBound.end())
      break;
  }
  updateGraphWithNewEdges(to_add_edges);
  updateGraphWithIsolatedEdges(to_remove_edges);
  cleanLeafNodes();
}
/*
 * Read: loopBound, flattenConfig, graph_, instid, dynamicMethodID,
 *       prevBasicBlock
 * Modify: graph_
 */
void BaseDatapath::storeBuffer() {
  if (loopBound.size() <= 2)
    return;
  std::cout << "-------------------------------" << std::endl;
  std::cout << "          Store Buffer         " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  std::vector<newEdge> to_add_edges;
  std::vector<unsigned> to_remove_nodes;

  auto bound_it = loopBound.begin();
  auto node_it = exec_nodes.begin();

  while (node_it != exec_nodes.end()) {
    while (node_it->first < bound_it->node_id && node_it != exec_nodes.end()) {
      ExecNode* node = node_it->second;
      if (!node->has_vertex() ||
          boost::degree(node->get_vertex(), graph_) == 0) {
        ++node_it;
        continue;
      }
      if (node->is_store_op()) {
        // remove this store, unless it is a dynamic store which cannot be
        // statically disambiguated.
        if (node->is_dynamic_mem_op()) {
          ++node_it;
          continue;
        }
        Vertex node_vertex = node->get_vertex();
        out_edge_iter out_edge_it, out_edge_end;

        std::vector<Vertex> store_child;
        for (boost::tie(out_edge_it, out_edge_end) =
                 out_edges(node_vertex, graph_);
             out_edge_it != out_edge_end;
             ++out_edge_it) {
          Vertex child_vertex = target(*out_edge_it, graph_);
          ExecNode* child_node = getNodeFromVertex(child_vertex);
          if (child_node->is_load_op()) {
            if (child_node->is_dynamic_mem_op() ||
                child_node->get_node_id() >= (unsigned)bound_it->node_id)
              continue;
            else
              store_child.push_back(child_vertex);
          }
        }

        if (store_child.size() > 0) {
          bool parent_found = false;
          Vertex store_parent;
          in_edge_iter in_edge_it, in_edge_end;
          for (boost::tie(in_edge_it, in_edge_end) =
                   in_edges(node_vertex, graph_);
               in_edge_it != in_edge_end;
               ++in_edge_it) {
            // parent node that generates value
            if (edge_to_parid[*in_edge_it] == 1) {
              parent_found = true;
              store_parent = source(*in_edge_it, graph_);
              break;
            }
          }

          if (parent_found) {
            for (auto load_it = store_child.begin(), E = store_child.end();
                 load_it != E;
                 ++load_it) {
              Vertex load_node = *load_it;
              to_remove_nodes.push_back(vertexToName[load_node]);

              out_edge_iter out_edge_it, out_edge_end;
              for (boost::tie(out_edge_it, out_edge_end) =
                       out_edges(load_node, graph_);
                   out_edge_it != out_edge_end;
                   ++out_edge_it) {
                Vertex target_vertex = target(*out_edge_it, graph_);
                to_add_edges.push_back({ getNodeFromVertex(store_parent),
                                         getNodeFromVertex(target_vertex),
                                         edge_to_parid[*out_edge_it] });
              }
            }
          }
        }
      }
      ++node_it;
    }
    ++bound_it;
    if (bound_it == loopBound.end())
      break;
  }
  updateGraphWithNewEdges(to_add_edges);
  updateGraphWithIsolatedNodes(to_remove_nodes);
  cleanLeafNodes();
}

/*
 * Read: loopBound, flattenConfig, graph_, address, instid, dynamicMethodID,
 *       prevBasicBlock
 * Modify: graph_
 */
void BaseDatapath::removeRepeatedStores() {
  if (!unrolling_config.size() && loopBound.size() <= 2)
    return;

  std::cout << "-------------------------------" << std::endl;
  std::cout << "     Remove Repeated Store     " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  int node_id = numTotalNodes - 1;
  auto bound_it = loopBound.end();
  bound_it--;
  bound_it--;
  while (node_id >= 0) {
    std::unordered_map<unsigned, int> address_store_map;

    while (node_id >= bound_it->node_id && node_id >= 0) {
      ExecNode* node = exec_nodes.at(node_id);
      if (!node->has_vertex() ||
          boost::degree(node->get_vertex(), graph_) == 0 ||
          !node->is_store_op()) {
        --node_id;
        continue;
      }
      long long int node_address = node->get_mem_access()->vaddr;
      auto addr_it = address_store_map.find(node_address);

      if (addr_it == address_store_map.end())
        address_store_map[node_address] = node_id;
      else {
        // remove this store, unless it is a dynamic store which cannot be
        // statically disambiguated.
        if (!node->is_dynamic_mem_op()) {
          if (boost::out_degree(node->get_vertex(), graph_) == 0) {
            node->set_microop(LLVM_IR_SilentStore);
          } else {
            int num_of_real_children = 0;
            out_edge_iter out_edge_it, out_edge_end;
            for (boost::tie(out_edge_it, out_edge_end) =
                     out_edges(node->get_vertex(), graph_);
                 out_edge_it != out_edge_end;
                 ++out_edge_it) {
              if (edge_to_parid[*out_edge_it] != CONTROL_EDGE)
                num_of_real_children++;
            }
            if (num_of_real_children == 0) {
              node->set_microop(LLVM_IR_SilentStore);
            }
          }
        }
      }
      --node_id;
    }

    if (bound_it == loopBound.begin())
      break;
    --bound_it;
  }
  cleanLeafNodes();
}
/*
 * Read: loopBound, flattenConfig, graph_
 * Modify: graph_
 */
void BaseDatapath::treeHeightReduction() {
  if (loopBound.size() <= 2)
    return;
  std::cout << "-------------------------------" << std::endl;
  std::cout << "     Tree Height Reduction     " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  std::map<unsigned, bool> updated;
  std::map<unsigned, int> bound_region;
  for (auto node_pair : exec_nodes) {
    updated[node_pair.first] = false;
    bound_region[node_pair.first] = 0;
  }

  int region_id = 0;
  auto node_it = exec_nodes.begin();
  unsigned node_id = node_it->first;
  auto bound_it = loopBound.begin();
  while (node_id <= bound_it->node_id && node_it != exec_nodes.end()) {
    bound_region.at(node_id) = region_id;
    if (node_id == bound_it->node_id) {
      region_id++;
      bound_it++;
      if (bound_it == loopBound.end())
        break;
    }
    node_it++;
    node_id = node_it->first;
  }

  std::set<Edge> to_remove_edges;
  std::vector<newEdge> to_add_edges;

  // nodes with no outgoing edges to first (bottom nodes first)
  for (auto node_it = exec_nodes.rbegin(); node_it != exec_nodes.rend();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (!node->has_vertex() || boost::degree(node->get_vertex(), graph_) == 0 ||
        updated.at(node->get_node_id()) || !node->is_associative())
      continue;
    unsigned node_id = node->get_node_id();
    updated.at(node_id) = 1;
    int node_region = bound_region.at(node_id);

    std::list<ExecNode*> nodes;
    std::vector<Edge> tmp_remove_edges;
    std::vector<std::pair<ExecNode*, bool>> leaves;
    std::vector<ExecNode*> associative_chain;
    associative_chain.push_back(node);
    int chain_id = 0;
    while (chain_id < associative_chain.size()) {
      ExecNode* chain_node = associative_chain[chain_id];
      if (chain_node->is_associative()) {
        updated.at(chain_node->get_node_id()) = 1;
        int num_of_chain_parents = 0;
        in_edge_iter in_edge_it, in_edge_end;
        for (boost::tie(in_edge_it, in_edge_end) =
                 in_edges(chain_node->get_vertex(), graph_);
             in_edge_it != in_edge_end;
             ++in_edge_it) {
          if (edge_to_parid[*in_edge_it] == CONTROL_EDGE)
            continue;
          num_of_chain_parents++;
        }
        if (num_of_chain_parents == 2) {
          nodes.push_front(chain_node);
          for (boost::tie(in_edge_it, in_edge_end) =
                   in_edges(chain_node->get_vertex(), graph_);
               in_edge_it != in_edge_end;
               ++in_edge_it) {
            if (edge_to_parid[*in_edge_it] == CONTROL_EDGE)
              continue;
            Vertex parent_vertex = source(*in_edge_it, graph_);
            int parent_id = vertexToName[parent_vertex];
            ExecNode* parent_node = getNodeFromVertex(parent_vertex);
            assert(*parent_node < *chain_node);

            Edge curr_edge = *in_edge_it;
            tmp_remove_edges.push_back(curr_edge);
            int parent_region = bound_region.at(parent_id);
            if (parent_region == node_region) {
              updated.at(parent_id) = 1;
              if (!parent_node->is_associative())
                leaves.push_back(std::make_pair(parent_node, false));
              else {
                out_edge_iter out_edge_it, out_edge_end;
                int num_of_children = 0;
                for (boost::tie(out_edge_it, out_edge_end) =
                         out_edges(parent_vertex, graph_);
                     out_edge_it != out_edge_end;
                     ++out_edge_it) {
                  if (edge_to_parid[*out_edge_it] != CONTROL_EDGE)
                    num_of_children++;
                }

                if (num_of_children == 1)
                  associative_chain.push_back(parent_node);
                else
                  leaves.push_back(std::make_pair(parent_node, false));
              }
            } else {
              leaves.push_back(std::make_pair(parent_node, true));
            }
          }
        } else {
          /* Promote the single parent node with higher priority. This affects
           * mostly the top of the graph where no parent exists. */
          leaves.push_back(std::make_pair(chain_node, true));
        }
      } else {
        leaves.push_back(std::make_pair(chain_node, false));
      }
      chain_id++;
    }
    // build the tree
    if (nodes.size() < 3)
      continue;
    for (auto it = tmp_remove_edges.begin(), E = tmp_remove_edges.end();
         it != E;
         it++)
      to_remove_edges.insert(*it);

    std::map<ExecNode*, unsigned> rank_map;
    auto leaf_it = leaves.begin();

    while (leaf_it != leaves.end()) {
      if (leaf_it->second == false)
        rank_map[leaf_it->first] = beginNodeId;
      else
        rank_map[leaf_it->first] = endNodeId;
      ++leaf_it;
    }
    // reconstruct the rest of the balanced tree
    // TODO: Find a better name for this.
    auto new_node_it = nodes.begin();
    while (new_node_it != nodes.end()) {
      ExecNode* node1 = nullptr;
      ExecNode* node2 = nullptr;
      if (rank_map.size() == 2) {
        node1 = rank_map.begin()->first;
        node2 = (++rank_map.begin())->first;
      } else {
        findMinRankNodes(&node1, &node2, rank_map);
      }
      // TODO: Is this at all possible...?
      assert((node1->get_node_id() != endNodeId) &&
             (node2->get_node_id() != endNodeId));
      to_add_edges.push_back({ node1, *new_node_it, 1 });
      to_add_edges.push_back({ node2, *new_node_it, 1 });

      // place the new node in the map, remove the two old nodes
      rank_map[*new_node_it] = std::max(rank_map[node1], rank_map[node2]) + 1;
      rank_map.erase(node1);
      rank_map.erase(node2);
      ++new_node_it;
    }
  }
  /*For tree reduction, we always remove edges first, then add edges. The reason
   * is that it's possible that we are adding the same edges as the edges that
   * we want to remove. Doing adding edges first then removing edges could lead
   * to losing dependences.*/
  updateGraphWithIsolatedEdges(to_remove_edges);
  updateGraphWithNewEdges(to_add_edges);
  cleanLeafNodes();
}

void BaseDatapath::findMinRankNodes(ExecNode** node1,
                                    ExecNode** node2,
                                    std::map<ExecNode*, unsigned>& rank_map) {
  unsigned min_rank = endNodeId;
  for (auto it = rank_map.begin(); it != rank_map.end(); ++it) {
    int node_rank = it->second;
    if (node_rank < min_rank) {
      *node1 = it->first;
      min_rank = node_rank;
    }
  }
  min_rank = endNodeId;
  for (auto it = rank_map.begin(); it != rank_map.end(); ++it) {
    int node_rank = it->second;
    if ((it->first != *node1) && (node_rank < min_rank)) {
      *node2 = it->first;
      min_rank = node_rank;
    }
  }
}

std::list<node_pair_t> BaseDatapath::findLoopBoundaries(
    const UniqueLabel& loop_label) {
  using namespace SrcTypes;
  std::list<node_pair_t> loop_boundaries;
  bool is_loop_executing = false;
  int current_loop_depth = -1;

  ExecNode* loop_start;

  // The loop boundaries provided by the accelerator are in a linear list with
  // no structure. We need to identify the start and end of each unrolled loop
  // section and add the corresponding pair of nodes to loop_boundaries.
  //
  // The last loop bound node is one past the last node, so we'll ignore it.
  for (auto it = loopBound.begin(); it != --loopBound.end(); ++it) {
    const DynLoopBound& loop_bound = *it;
    ExecNode* node = exec_nodes[loop_bound.node_id];
    Vertex vertex = node->get_vertex();
    if (boost::degree(vertex, graph_) > 0 &&
        node->get_line_num() == loop_label.get_line_number() &&
        node->get_static_function() == loop_label.get_function()) {
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

std::list<node_pair_t> BaseDatapath::findFunctionBoundaries(
    SrcTypes::Function* func) {
  std::list<node_pair_t> boundaries;
  if (!func)
    return boundaries;

  node_pair_t current_bound;
  for (auto& node_id_pair : exec_nodes) {
    ExecNode* node = node_id_pair.second;
    if (node->is_call_op()) {
      ExecNode* next_node = getNextNode(node->get_node_id());
      if (next_node->get_static_function() == func)
        current_bound.first = node;
    } else if (node->is_ret_op() && node->get_static_function() == func) {
      current_bound.second = node;
      boundaries.push_back(current_bound);
    }
  }
  return boundaries;
}

void BaseDatapath::updateGraphWithNewEdges(std::vector<newEdge>& to_add_edges) {
  std::cout << "  Adding " << to_add_edges.size() << " new edges.\n";
  for (auto it = to_add_edges.begin(); it != to_add_edges.end(); ++it) {
    if (*it->from != *it->to && !doesEdgeExist(it->from, it->to)) {
      get(boost::edge_name, graph_)[add_edge(
          it->from->get_vertex(), it->to->get_vertex(), graph_).first] =
          it->parid;
    }
  }
}
void BaseDatapath::updateGraphWithIsolatedNodes(
    std::vector<unsigned>& to_remove_nodes) {
  std::cout << "  Removing " << to_remove_nodes.size() << " isolated nodes.\n";
  for (auto it = to_remove_nodes.begin(); it != to_remove_nodes.end(); ++it) {
    clear_vertex(exec_nodes.at(*it)->get_vertex(), graph_);
  }
}

void BaseDatapath::updateGraphWithIsolatedEdges(std::set<Edge>& to_remove_edges) {
  std::cout << "  Removing " << to_remove_edges.size() << " edges.\n";
  for (auto it = to_remove_edges.begin(), E = to_remove_edges.end(); it != E;
       ++it)
    remove_edge(*it, graph_);
}

/*
 * Write per cycle activity to bench_stats. The format is:
 * cycle_num,num-of-muls,num-of-adds,num-of-bitwise-ops,num-of-reg-reads,num-of-reg-writes
 * If it is called from ScratchpadDatapath, it also outputs per cycle memory
 * activity for each partitioned array.
 */
void BaseDatapath::writePerCycleActivity() {
  /* Activity per function in the code. Indexed by function names.  */
  std::unordered_map<std::string, std::vector<funcActivity>> func_activity;
  std::unordered_map<std::string, funcActivity> func_max_activity;
  /* Activity per array. Indexed by array names.  */
  std::unordered_map<std::string, std::vector<memActivity>> mem_activity;

  std::vector<std::string> comp_partition_names;
  std::vector<std::string> mem_partition_names;
  registers.getRegisterNames(comp_partition_names);
  getMemoryBlocks(mem_partition_names);

  initPerCycleActivity(comp_partition_names,
                       mem_partition_names,
                       mem_activity,
                       func_activity,
                       func_max_activity,
                       num_cycles);

  updatePerCycleActivity(mem_activity, func_activity, func_max_activity);

  outputPerCycleActivity(comp_partition_names,
                         mem_partition_names,
                         mem_activity,
                         func_activity,
                         func_max_activity);
}

void BaseDatapath::initPerCycleActivity(
    std::vector<std::string>& comp_partition_names,
    std::vector<std::string>& mem_partition_names,
    std::unordered_map<std::string, std::vector<memActivity>>& mem_activity,
    std::unordered_map<std::string, std::vector<funcActivity>>& func_activity,
    std::unordered_map<std::string, funcActivity>& func_max_activity,
    int num_cycles) {
  for (auto it = comp_partition_names.begin(); it != comp_partition_names.end();
       ++it) {
    mem_activity.insert(
        { *it, std::vector<memActivity>(num_cycles, { 0, 0 }) });
  }
  for (auto it = mem_partition_names.begin(); it != mem_partition_names.end();
       ++it) {
    mem_activity.insert(
        { *it, std::vector<memActivity>(num_cycles, { 0, 0 }) });
  }
  for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
    funcActivity tmp;
    func_activity.insert({ *it, std::vector<funcActivity>(num_cycles, tmp) });
    func_max_activity.insert({ *it, tmp });
  }
}

void BaseDatapath::updatePerCycleActivity(
    std::unordered_map<std::string, std::vector<memActivity>>& mem_activity,
    std::unordered_map<std::string, std::vector<funcActivity>>& func_activity,
    std::unordered_map<std::string, funcActivity>& func_max_activity) {
  /* We use two ways to count the number of functional units in accelerators:
   * one assumes that functional units can be reused in the same region; the
   * other assumes no reuse of functional units. The advantage of reusing is
   * that it eliminates the cost of duplicating functional units which can lead
   * to high leakage power and area. However, additional wires and muxes may
   * need to be added for reusing.
   * In the current model, we assume that multipliers can be reused, since the
   * leakage power and area of multipliers are relatively significant, and no
   * reuse for adders. This way of modeling is consistent with our observation
   * of accelerators generated with Vivado. */
  int num_adds_so_far = 0, num_bits_so_far = 0;
  int num_shifters_so_far = 0;
  auto bound_it = loopBound.begin();
  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    // TODO: On every iteration, this could be a bottleneck.
    std::string func_id = node->get_static_function()->get_name();
    auto max_it = func_max_activity.find(func_id);
    assert(max_it != func_max_activity.end());

    if (node->get_node_id() == bound_it->node_id) {
      if (max_it->second.add < num_adds_so_far)
        max_it->second.add = num_adds_so_far;
      if (max_it->second.bit < num_bits_so_far)
        max_it->second.bit = num_bits_so_far;
      if (max_it->second.shifter < num_shifters_so_far)
        max_it->second.shifter = num_shifters_so_far;
      num_adds_so_far = 0;
      num_bits_so_far = 0;
      num_shifters_so_far = 0;
      bound_it++;
    }
    if (node->is_isolated())
      continue;
    int node_level = node->get_start_execution_cycle();
    funcActivity& curr_fu_activity = func_activity.at(func_id).at(node_level);

    if (node->is_multicycle_op()) {
      for (unsigned stage = 0;
           node_level + stage < node->get_complete_execution_cycle();
           stage++) {
        funcActivity& fp_fu_activity =
            func_activity.at(func_id).at(node_level + stage);
        /* Activity for floating point functional units includes all their
         * stages.*/
        if (node->is_fp_add_op()) {
          if (node->is_double_precision())
            fp_fu_activity.fp_dp_add += 1;
          else
            fp_fu_activity.fp_sp_add += 1;
        } else if (node->is_fp_mul_op()) {
          if (node->is_double_precision())
            fp_fu_activity.fp_dp_mul += 1;
          else
            fp_fu_activity.fp_sp_mul += 1;
        } else if (node->is_trig_op()) {
          fp_fu_activity.trig += 1;
        }
      }
    } else if (node->is_int_mul_op()) {
      curr_fu_activity.mul += 1;
    } else if (node->is_int_add_op()) {
      curr_fu_activity.add += 1;
      num_adds_so_far += 1;
    } else if (node->is_shifter_op()) {
      curr_fu_activity.shifter += 1;
      num_shifters_so_far += 1;
    } else if (node->is_bit_op()) {
      curr_fu_activity.bit += 1;
      num_bits_so_far += 1;
    } else if (node->is_load_op()) {
      const std::string& array_label = node->get_array_label();
      if (mem_activity.find(array_label) != mem_activity.end())
        mem_activity.at(array_label).at(node_level).read += 1;
    } else if (node->is_store_op()) {
      const std::string& array_label = node->get_array_label();
      if (mem_activity.find(array_label) != mem_activity.end())
        mem_activity.at(array_label).at(node_level).write += 1;
    }
  }
  for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
    auto max_it = func_max_activity.find(*it);
    assert(max_it != func_max_activity.end());
    std::vector<funcActivity>& cycle_activity = func_activity.at(*it);
    max_it->second.mul =
        std::max_element(cycle_activity.begin(),
                         cycle_activity.end(),
                         [](const funcActivity& a, const funcActivity& b) {
                           return (a.mul < b.mul);
                         })->mul;
    max_it->second.fp_sp_mul =
        std::max_element(cycle_activity.begin(),
                         cycle_activity.end(),
                         [](const funcActivity& a, const funcActivity& b) {
                           return (a.fp_sp_mul < b.fp_sp_mul);
                         })->fp_sp_mul;
    max_it->second.fp_dp_mul =
        std::max_element(cycle_activity.begin(),
                         cycle_activity.end(),
                         [](const funcActivity& a, const funcActivity& b) {
                           return (a.fp_dp_mul < b.fp_dp_mul);
                         })->fp_dp_mul;
    max_it->second.fp_sp_add =
        std::max_element(cycle_activity.begin(),
                         cycle_activity.end(),
                         [](const funcActivity& a, const funcActivity& b) {
                           return (a.fp_sp_add < b.fp_sp_add);
                         })->fp_sp_add;
    max_it->second.fp_dp_add =
        std::max_element(cycle_activity.begin(),
                         cycle_activity.end(),
                         [](const funcActivity& a, const funcActivity& b) {
                           return (a.fp_dp_add < b.fp_dp_add);
                         })->fp_dp_add;
    max_it->second.trig =
        std::max_element(cycle_activity.begin(),
                         cycle_activity.end(),
                         [](const funcActivity& a, const funcActivity& b) {
                           return (a.trig < b.trig);
                         })->trig;
  }
}

void BaseDatapath::outputPerCycleActivity(
    std::vector<std::string>& comp_partition_names,
    std::vector<std::string>& mem_partition_names,
    std::unordered_map<std::string, std::vector<memActivity>>& mem_activity,
    std::unordered_map<std::string, std::vector<funcActivity>>& func_activity,
    std::unordered_map<std::string, funcActivity>& func_max_activity) {
  /*Set the constants*/
  float add_int_power, add_switch_power, add_leak_power, add_area;
  float mul_int_power, mul_switch_power, mul_leak_power, mul_area;
  float fp_sp_mul_int_power, fp_sp_mul_switch_power, fp_sp_mul_leak_power,
      fp_sp_mul_area;
  float fp_dp_mul_int_power, fp_dp_mul_switch_power, fp_dp_mul_leak_power,
      fp_dp_mul_area;
  float fp_sp_add_int_power, fp_sp_add_switch_power, fp_sp_add_leak_power,
      fp_sp_add_area;
  float fp_dp_add_int_power, fp_dp_add_switch_power, fp_dp_add_leak_power,
      fp_dp_add_area;
  float trig_int_power, trig_switch_power, trig_leak_power, trig_area;
  float reg_int_power_per_bit, reg_switch_power_per_bit;
  float reg_leak_power_per_bit, reg_area_per_bit;
  float bit_int_power, bit_switch_power, bit_leak_power, bit_area;
  float shifter_int_power, shifter_switch_power, shifter_leak_power,
      shifter_area;
  unsigned idle_fu_cycles = 0;

  getAdderPowerArea(
      cycleTime, &add_int_power, &add_switch_power, &add_leak_power, &add_area);
  getMultiplierPowerArea(
      cycleTime, &mul_int_power, &mul_switch_power, &mul_leak_power, &mul_area);
  getRegisterPowerArea(cycleTime,
                       &reg_int_power_per_bit,
                       &reg_switch_power_per_bit,
                       &reg_leak_power_per_bit,
                       &reg_area_per_bit);
  getBitPowerArea(
      cycleTime, &bit_int_power, &bit_switch_power, &bit_leak_power, &bit_area);
  getShifterPowerArea(cycleTime,
                      &shifter_int_power,
                      &shifter_switch_power,
                      &shifter_leak_power,
                      &shifter_area);
  getSinglePrecisionFloatingPointMultiplierPowerArea(cycleTime,
                                                     &fp_sp_mul_int_power,
                                                     &fp_sp_mul_switch_power,
                                                     &fp_sp_mul_leak_power,
                                                     &fp_sp_mul_area);
  getDoublePrecisionFloatingPointMultiplierPowerArea(cycleTime,
                                                     &fp_dp_mul_int_power,
                                                     &fp_dp_mul_switch_power,
                                                     &fp_dp_mul_leak_power,
                                                     &fp_dp_mul_area);
  getSinglePrecisionFloatingPointAdderPowerArea(cycleTime,
                                                &fp_sp_add_int_power,
                                                &fp_sp_add_switch_power,
                                                &fp_sp_add_leak_power,
                                                &fp_sp_add_area);
  getDoublePrecisionFloatingPointAdderPowerArea(cycleTime,
                                                &fp_dp_add_int_power,
                                                &fp_dp_add_switch_power,
                                                &fp_dp_add_leak_power,
                                                &fp_dp_add_area);
  getTrigonometricFunctionPowerArea(cycleTime,
                                    &trig_int_power,
                                    &trig_switch_power,
                                    &trig_leak_power,
                                    &trig_area);

  std::string file_name;
#ifdef DEBUG
  file_name = benchName + "_stats";
  std::ofstream stats;
  stats.open(file_name.c_str(), std::ofstream::out | std::ofstream::app);
  stats << "cycles," << num_cycles << "," << numTotalNodes << std::endl;
  stats << num_cycles << ",";

  std::ofstream power_stats;
  file_name += "_power";
  power_stats.open(file_name.c_str(), std::ofstream::out | std::ofstream::app);
  power_stats << "cycles," << num_cycles << "," << numTotalNodes << std::endl;
  power_stats << num_cycles << ",";

  /*Start writing the second line*/
  for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
    stats << *it << "-fp-sp-mul," << *it << "-fp-dp-mul," << *it
          << "-fp-sp-add," << *it << "-fp-dp-add," << *it << "-mul," << *it
          << "-add," << *it << "-bit," << *it << "-shifter," << *it << "-trig,";
    power_stats << *it << "-fp-sp-mul," << *it << "-fp-dp-mul," << *it
                << "-fp-sp-add," << *it << "-fp-dp-add," << *it << "-mul,"
                << *it << "-add," << *it << "-bit," << *it << "-shifter," << *it
                << "-trig,";
  }
  // TODO: mem_partition_names contains logical arrays, not completely
  // partitioned arrays.
  stats << "reg,";
  for (auto it = mem_partition_names.begin(); it != mem_partition_names.end();
       ++it) {
    stats << *it << "-read," << *it << "-write,";
  }
  stats << std::endl;
  power_stats << "reg,";
  power_stats << std::endl;
#endif
  /*Finish writing the second line*/

  /*Caculating the number of FUs and leakage power*/
  int max_reg_read = 0, max_reg_write = 0;
  for (unsigned level_id = 0; ((int)level_id) < num_cycles; ++level_id) {
    if (max_reg_read < regStats.at(level_id).reads)
      max_reg_read = regStats.at(level_id).reads;
    if (max_reg_write < regStats.at(level_id).writes)
      max_reg_write = regStats.at(level_id).writes;
  }
  int max_reg = max_reg_read + max_reg_write;
  int max_add = 0, max_mul = 0, max_bit = 0, max_shifter = 0;
  int max_fp_sp_mul = 0, max_fp_dp_mul = 0;
  int max_fp_sp_add = 0, max_fp_dp_add = 0;
  int max_trig = 0;
  for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
    auto max_it = func_max_activity.find(*it);
    assert(max_it != func_max_activity.end());
    max_bit += max_it->second.bit;
    max_add += max_it->second.add;
    max_mul += max_it->second.mul;
    max_shifter += max_it->second.shifter;
    max_fp_sp_mul += max_it->second.fp_sp_mul;
    max_fp_dp_mul += max_it->second.fp_dp_mul;
    max_fp_sp_add += max_it->second.fp_sp_add;
    max_fp_dp_add += max_it->second.fp_dp_add;
    max_trig += max_it->second.trig;
  }

  float add_leakage_power = add_leak_power * max_add;
  float mul_leakage_power = mul_leak_power * max_mul;
  float bit_leakage_power = bit_leak_power * max_bit;
  float shifter_leakage_power = shifter_leak_power * max_shifter;
  float reg_leakage_power =
      registers.getTotalLeakagePower() + reg_leak_power_per_bit * 32 * max_reg;
  float fp_sp_mul_leakage_power = fp_sp_mul_leak_power * max_fp_sp_mul;
  float fp_dp_mul_leakage_power = fp_dp_mul_leak_power * max_fp_dp_mul;
  float fp_sp_add_leakage_power = fp_sp_add_leak_power * max_fp_sp_add;
  float fp_dp_add_leakage_power = fp_dp_add_leak_power * max_fp_dp_add;
  float trig_leakage_power = trig_leak_power * max_trig;
  float fu_leakage_power = mul_leakage_power + add_leakage_power +
                           reg_leakage_power + bit_leakage_power +
                           shifter_leakage_power + fp_sp_mul_leakage_power +
                           fp_dp_mul_leakage_power + fp_sp_add_leakage_power +
                           fp_dp_add_leakage_power + trig_leakage_power;
  /*Finish calculating the number of FUs and leakage power*/

  float fu_dynamic_energy = 0;

  /*Start writing per cycle activity */
  for (unsigned curr_level = 0; ((int)curr_level) < num_cycles; ++curr_level) {
#ifdef DEBUG
    stats << curr_level << ",";
    power_stats << curr_level << ",";
#endif
    bool is_fu_idle = true;
    // For FUs
    for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
      funcActivity& curr_activity = func_activity.at(*it).at(curr_level);
      is_fu_idle &= curr_activity.is_idle();
#ifdef DEBUG
      stats << curr_activity.fp_sp_mul << ","
            << curr_activity.fp_dp_mul << ","
            << curr_activity.fp_sp_add << ","
            << curr_activity.fp_dp_add << ","
            << curr_activity.mul << ","
            << curr_activity.add << ","
            << curr_activity.bit << ","
            << curr_activity.shifter << ","
            << curr_activity.trig << ",";
#endif
      float curr_fp_sp_mul_dynamic_power =
          (fp_sp_mul_switch_power + fp_sp_mul_int_power) *
          curr_activity.fp_sp_mul;
      float curr_fp_dp_mul_dynamic_power =
          (fp_dp_mul_switch_power + fp_dp_mul_int_power) *
          curr_activity.fp_dp_mul;
      float curr_fp_sp_add_dynamic_power =
          (fp_sp_add_switch_power + fp_sp_add_int_power) *
          curr_activity.fp_sp_add;
      float curr_fp_dp_add_dynamic_power =
          (fp_dp_add_switch_power + fp_dp_add_int_power) *
          curr_activity.fp_dp_add;
      float curr_trig_dynamic_power =
          (trig_switch_power + trig_int_power) *
          curr_activity.trig;
      float curr_mul_dynamic_power = (mul_switch_power + mul_int_power) *
                                     curr_activity.mul;
      float curr_add_dynamic_power = (add_switch_power + add_int_power) *
                                     curr_activity.add;
      float curr_bit_dynamic_power = (bit_switch_power + bit_int_power) *
                                     curr_activity.bit;
      float curr_shifter_dynamic_power =
          (shifter_switch_power + shifter_int_power) *
          curr_activity.shifter;
      fu_dynamic_energy +=
          (curr_fp_sp_mul_dynamic_power + curr_fp_dp_mul_dynamic_power +
           curr_fp_sp_add_dynamic_power + curr_fp_dp_add_dynamic_power +
           curr_trig_dynamic_power + curr_mul_dynamic_power +
           curr_add_dynamic_power + curr_bit_dynamic_power +
           curr_shifter_dynamic_power) *
          cycleTime;
#ifdef DEBUG
      power_stats << curr_mul_dynamic_power + mul_leakage_power << ","
                  << curr_add_dynamic_power + add_leakage_power << ","
                  << curr_bit_dynamic_power + bit_leakage_power << ","
                  << curr_shifter_dynamic_power + shifter_leakage_power << ","
                  << curr_trig_dynamic_power << ",";
#endif
    }
    // For regs
    int curr_reg_reads = regStats.at(curr_level).reads;
    int curr_reg_writes = regStats.at(curr_level).writes;
    float curr_reg_dynamic_energy =
        (reg_int_power_per_bit + reg_switch_power_per_bit) *
        (curr_reg_reads + curr_reg_writes) * 32 * cycleTime;
    for (auto it = comp_partition_names.begin();
         it != comp_partition_names.end();
         ++it) {
      curr_reg_reads += mem_activity.at(*it).at(curr_level).read;
      curr_reg_writes += mem_activity.at(*it).at(curr_level).write;
      curr_reg_dynamic_energy += registers.getReadEnergy(*it) *
                                     mem_activity.at(*it).at(curr_level).read +
                                 registers.getWriteEnergy(*it) *
                                     mem_activity.at(*it).at(curr_level).write;
    }
    fu_dynamic_energy += curr_reg_dynamic_energy;

#ifdef DEBUG
    stats << curr_reg_reads << "," << curr_reg_writes << ",";

    for (auto it = mem_partition_names.begin(); it != mem_partition_names.end();
         ++it) {
      stats << mem_activity.at(*it).at(curr_level).read << ","
            << mem_activity.at(*it).at(curr_level).write << ",";
    }
    stats << std::endl;
    power_stats << curr_reg_dynamic_energy / cycleTime + reg_leakage_power;
    power_stats << std::endl;
#endif
    if (is_fu_idle)
      idle_fu_cycles++;
  }
#ifdef DEBUG
  stats.close();
  power_stats.close();
#endif

  float avg_mem_power = 0, avg_mem_dynamic_power = 0, mem_leakage_power = 0;

  getAverageMemPower(
      num_cycles, &avg_mem_power, &avg_mem_dynamic_power, &mem_leakage_power);

  float avg_fu_dynamic_power = fu_dynamic_energy / (cycleTime * num_cycles);
  float avg_fu_power = avg_fu_dynamic_power + fu_leakage_power;
  float avg_power = avg_fu_power + avg_mem_power;

  float mem_area = getTotalMemArea();
  float fu_area =
      registers.getTotalArea() + add_area * max_add + mul_area * max_mul +
      reg_area_per_bit * 32 * max_reg + bit_area * max_bit +
      shifter_area * max_shifter + fp_sp_mul_area * max_fp_sp_mul +
      fp_dp_mul_area * max_fp_dp_mul + fp_sp_add_area * max_fp_sp_add +
      fp_dp_add_area * max_fp_dp_add + trig_area * max_trig;
  float total_area = mem_area + fu_area;

  // Summary output.
  summary_data_t summary;
  summary.benchName = benchName;
  summary.num_cycles = num_cycles;
  summary.idle_fu_cycles = idle_fu_cycles;
  summary.avg_power = avg_power;
  summary.avg_fu_power = avg_fu_power;
  summary.avg_fu_dynamic_power = avg_fu_dynamic_power;
  summary.fu_leakage_power = fu_leakage_power;
  summary.avg_mem_power = avg_mem_power;
  summary.avg_mem_dynamic_power = avg_mem_dynamic_power;
  summary.mem_leakage_power = mem_leakage_power;
  summary.total_area = total_area;
  summary.fu_area = fu_area;
  summary.mem_area = mem_area;
  summary.max_mul = max_mul;
  summary.max_add = max_add;
  summary.max_bit = max_bit;
  summary.max_shifter = max_shifter;
  summary.max_reg = max_reg;
  summary.max_fp_sp_mul = max_fp_sp_mul;
  summary.max_fp_dp_mul = max_fp_dp_mul;
  summary.max_fp_sp_add = max_fp_sp_add;
  summary.max_fp_dp_add = max_fp_dp_add;
  summary.max_trig = max_trig;

  writeSummary(std::cout, summary);
  std::ofstream summary_file;
  file_name = benchName + "_summary";
  summary_file.open(file_name.c_str(), std::ofstream::out | std::ofstream::app);
  writeSummary(summary_file, summary);
  summary_file.close();

#ifdef USE_DB
  if (use_db)
    writeSummaryToDatabase(summary);
#endif
}

void BaseDatapath::writeSummary(std::ostream& outfile,
                                summary_data_t& summary) {
  outfile << "===============================" << std::endl;
  outfile << "        Aladdin Results        " << std::endl;
  outfile << "===============================" << std::endl;
  outfile << "Running : " << summary.benchName << std::endl;
  outfile << "Cycle : " << summary.num_cycles << " cycles" << std::endl;
  outfile << "Avg Power: " << summary.avg_power << " mW" << std::endl;
  outfile << "Idle FU Cycles: " << summary.idle_fu_cycles << " cycles" << std::endl;
  outfile << "Avg FU Power: " << summary.avg_fu_power << " mW" << std::endl;
  outfile << "Avg FU Dynamic Power: " << summary.avg_fu_dynamic_power << " mW"
          << std::endl;
  outfile << "Avg FU leakage Power: " << summary.fu_leakage_power << " mW"
          << std::endl;
  outfile << "Avg MEM Power: " << summary.avg_mem_power << " mW" << std::endl;
  outfile << "Avg MEM Dynamic Power: " << summary.avg_mem_dynamic_power << " mW"
          << std::endl;
  outfile << "Avg MEM Leakage Power: " << summary.mem_leakage_power << " mW"
          << std::endl;
  outfile << "Total Area: " << summary.total_area << " uM^2" << std::endl;
  outfile << "FU Area: " << summary.fu_area << " uM^2" << std::endl;
  outfile << "MEM Area: " << summary.mem_area << " uM^2" << std::endl;
  if (summary.max_fp_sp_mul != 0)
    outfile << "Num of Single Precision FP Multipliers: "
            << summary.max_fp_sp_mul << std::endl;
  if (summary.max_fp_sp_add != 0)
    outfile << "Num of Single Precision FP Adders: " << summary.max_fp_sp_add
            << std::endl;
  if (summary.max_fp_dp_mul != 0)
    outfile << "Num of Double Precision FP Multipliers: "
            << summary.max_fp_dp_mul << std::endl;
  if (summary.max_fp_dp_add != 0)
    outfile << "Num of Double Precision FP Adders: " << summary.max_fp_dp_add
            << std::endl;
  if (summary.max_trig != 0)
    outfile << "Num of Trigonometric Units: " << summary.max_trig << std::endl;
  if (summary.max_mul != 0)
    outfile << "Num of Multipliers (32-bit): " << summary.max_mul << std::endl;
  if (summary.max_add != 0)
    outfile << "Num of Adders (32-bit): " << summary.max_add << std::endl;
  if (summary.max_bit != 0)
    outfile << "Num of Bit-wise Operators (32-bit): " << summary.max_bit
            << std::endl;
  if (summary.max_shifter != 0)
    outfile << "Num of Shifters (32-bit): " << summary.max_shifter << std::endl;
  outfile << "Num of Registers (32-bit): " << summary.max_reg << std::endl;
  outfile << "===============================" << std::endl;
  outfile << "        Aladdin Results        " << std::endl;
  outfile << "===============================" << std::endl;
}

void BaseDatapath::writeBaseAddress() {
  std::ostringstream file_name;
  file_name << benchName << "_baseAddr.gz";
  gzFile gzip_file;
  gzip_file = gzopen(file_name.str().c_str(), "w");
  for (auto it = exec_nodes.begin(), E = exec_nodes.end(); it != E; ++it) {
    char original_label[256];
    int partition_id;
    std::string partitioned_label = it->second->get_array_label();
    if (partitioned_label.empty())
      continue;
    int num_fields = sscanf(
        partitioned_label.c_str(), "%[^-]-%d", original_label, &partition_id);
    if (num_fields != 2)
      continue;
    gzprintf(gzip_file,
             "node:%u,part:%s,base:%lld\n",
             it->first,
             partitioned_label.c_str(),
             getBaseAddress(original_label));
  }
  gzclose(gzip_file);
}

void BaseDatapath::writeOtherStats() {
  // First collect the data from exec_nodes.
  std::vector<int> microop;
  std::vector<int> exec_cycle;
  std::vector<bool> isolated;
  microop.reserve(totalConnectedNodes);
  exec_cycle.reserve(totalConnectedNodes);
  isolated.reserve(totalConnectedNodes);

  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    microop.push_back(node->get_microop());
    exec_cycle.push_back(node->get_start_execution_cycle());
    isolated.push_back(node->is_isolated());
  }

  std::string cycle_file_name(benchName);
  cycle_file_name += "_level.gz";
  write_gzip_file(cycle_file_name, exec_cycle.size(), exec_cycle);

  std::string microop_file_name(benchName);
  microop_file_name += "_microop.gz";
  write_gzip_file(microop_file_name, microop.size(), microop);

  std::string isolated_file_name(benchName);
  microop_file_name += "_isolated.gz";
  write_gzip_bool_file(isolated_file_name, isolated.size(), isolated);
}

// stepFunctions
// multiple function, each function is a separate graph
void BaseDatapath::prepareForScheduling() {
  std::cout << "=============================================" << std::endl;
  std::cout << "      Scheduling...            " << benchName << std::endl;
  std::cout << "=============================================" << std::endl;

  edgeToParid = get(boost::edge_name, graph_);

  numTotalEdges = boost::num_edges(graph_);
  executedNodes = 0;
  totalConnectedNodes = 0;
  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (!node->has_vertex())
      continue;
    Vertex node_vertex = node->get_vertex();
    if (boost::degree(node_vertex, graph_) != 0 || node->is_dma_op()) {
      node->set_num_parents(boost::in_degree(node_vertex, graph_));
      node->set_isolated(false);
      totalConnectedNodes++;
    }
  }
  std::cout << "  Total connected nodes: " << totalConnectedNodes << "\n";
  std::cout << "  Total edges: " << numTotalEdges << "\n";
  std::cout << "=============================================" << std::endl;

  executingQueue.clear();
  readyToExecuteQueue.clear();
  initExecutingQueue();
}

void BaseDatapath::dumpGraph(std::string graph_name) {
  std::unordered_map<Vertex, ExecNode*> vertexToMicroop;
  BGL_FORALL_VERTICES(v, graph_, Graph) {
    ExecNode* node = exec_nodes.at(get(boost::vertex_node_id, graph_, v));
    vertexToMicroop[v] = node;
  }
  std::ofstream out(
      graph_name + "_graph.dot", std::ofstream::out | std::ofstream::app);
  write_graphviz(out, graph_, make_microop_label_writer(vertexToMicroop, graph_));
}

/*As Late As Possible (ALAP) rescheduling for non-memory, non-control nodes.
  The first pass of scheduling is as early as possible, whenever a node's
  parents are ready, the node is executed. This mode of executing potentially
  can lead to values that are produced way earlier than they are needed. For
  such case, we add an ALAP rescheduling pass to reorganize the graph without
  changing the critical path and memory nodes, but produce a more balanced
  design.*/
int BaseDatapath::rescheduleNodesWhenNeeded() {
  std::vector<Vertex> topo_nodes;
  boost::topological_sort(graph_, std::back_inserter(topo_nodes));
  // bottom nodes first
  std::map<unsigned, int> earliest_child;
  for (auto node_id_pair : exec_nodes) {
    earliest_child[node_id_pair.first] = num_cycles;
  }
  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi) {
    unsigned node_id = vertexToName[*vi];
    ExecNode* node = exec_nodes.at(node_id);
    if (node->is_isolated())
      continue;
    if (!node->is_memory_op() && !node->is_branch_op()) {
      int new_cycle = earliest_child.at(node_id) - 1;
      if (new_cycle > node->get_complete_execution_cycle()) {
        node->set_complete_execution_cycle(new_cycle);
        if (node->is_fp_op()) {
          node->set_start_execution_cycle(
              new_cycle - node->fp_node_latency_in_cycles() + 1);
        } else {
          node->set_start_execution_cycle(new_cycle);
        }
      }
    }

    in_edge_iter in_i, in_end;
    for (boost::tie(in_i, in_end) = in_edges(*vi, graph_); in_i != in_end;
         ++in_i) {
      int parent_id = vertexToName[source(*in_i, graph_)];
      if (earliest_child.at(parent_id) > node->get_start_execution_cycle())
        earliest_child.at(parent_id) = node->get_start_execution_cycle();
    }
  }
  return num_cycles;
}

void BaseDatapath::updateRegStats() {
  regStats.assign(num_cycles, { 0, 0, 0 });
  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (node->is_isolated() || node->is_control_op() || node->is_index_op())
      continue;
    int node_level = node->get_complete_execution_cycle();
    int max_children_level = node_level;

    Vertex node_vertex = node->get_vertex();
    out_edge_iter out_edge_it, out_edge_end;
    std::set<int> children_levels;
    for (boost::tie(out_edge_it, out_edge_end) = out_edges(node_vertex, graph_);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      int child_id = vertexToName[target(*out_edge_it, graph_)];
      ExecNode* child_node = exec_nodes.at(child_id);
      if (child_node->is_control_op() || child_node->is_load_op())
        continue;

      int child_level = child_node->get_start_execution_cycle();
      if (child_level > max_children_level)
        max_children_level = child_level;
      if (child_level > node_level && child_level != num_cycles - 1)
        children_levels.insert(child_level);
    }
    for (auto it = children_levels.begin(); it != children_levels.end(); it++)
      regStats.at(*it).reads++;

    if (max_children_level > node_level && node_level != 0)
      regStats.at(node_level).writes++;
  }
}

void BaseDatapath::copyToExecutingQueue() {
  auto it = readyToExecuteQueue.begin();
  while (it != readyToExecuteQueue.end()) {
    ExecNode* node = *it;
    if (node->is_store_op())
      executingQueue.push_front(node);
    else
      executingQueue.push_back(node);
    it = readyToExecuteQueue.erase(it);
  }
}

bool BaseDatapath::step() {
  stepExecutingQueue();
  copyToExecutingQueue();
  num_cycles++;
  if (executedNodes == totalConnectedNodes)
    return true;
  return false;
}

void BaseDatapath::markNodeStarted(ExecNode* node) {
  node->set_start_execution_cycle(num_cycles);
}
// Marks a node as completed and advances the executing queue iterator.
void BaseDatapath::markNodeCompleted(
    std::list<ExecNode*>::iterator& executingQueuePos, int& advance_to) {
  ExecNode* node = *executingQueuePos;
  executedNodes++;
  node->set_complete_execution_cycle(num_cycles);
  executingQueue.erase(executingQueuePos);
  updateChildren(node);
  executingQueuePos = executingQueue.begin();
  std::advance(executingQueuePos, advance_to);
}

void BaseDatapath::updateChildren(ExecNode* node) {
  if (!node->has_vertex())
    return;
  Vertex node_vertex = node->get_vertex();
  out_edge_iter out_edge_it, out_edge_end;
  for (boost::tie(out_edge_it, out_edge_end) = out_edges(node_vertex, graph_);
       out_edge_it != out_edge_end;
       ++out_edge_it) {
    Vertex child_vertex = target(*out_edge_it, graph_);
    ExecNode* child_node = getNodeFromVertex(child_vertex);
    int edge_parid = edgeToParid[*out_edge_it];
    if (child_node->get_num_parents() > 0) {
      child_node->decr_num_parents();
      if (child_node->get_num_parents() == 0) {
        bool child_zero_latency =
            (child_node->is_memory_op())
                ? false
                : (child_node->fu_node_latency(cycleTime) == 0);
        bool curr_zero_latency = (node->is_memory_op())
                                     ? false
                                     : (node->fu_node_latency(cycleTime) == 0);
        if (edge_parid == REGISTER_EDGE || edge_parid == FUSED_BRANCH_EDGE ||
            ((child_zero_latency || curr_zero_latency) &&
             edge_parid != CONTROL_EDGE)) {
          executingQueue.push_back(child_node);
        } else {
          if (child_node->is_store_op())
            readyToExecuteQueue.push_front(child_node);
          else
            readyToExecuteQueue.push_back(child_node);
        }
        child_node->set_num_parents(-1);
      }
    }
  }
}

/*
 * Read: graph, getElementPtr.gz, completePartitionConfig, PartitionConfig
 * Modify: baseAddress
 */
void BaseDatapath::initBaseAddress() {
  std::cout << "-------------------------------" << std::endl;
  std::cout << "       Init Base Address       " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  vertex_iter vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(graph_); vi != vi_end; ++vi) {
    if (boost::degree(*vi, graph_) == 0)
      continue;
    Vertex curr_vertex = *vi;
    ExecNode* node = getNodeFromVertex(curr_vertex);
    if (!node->is_memory_op())
      continue;
    int node_microop = node->get_microop();
    // iterate its parents, until it finds the root parent
    while (true) {
      bool found_parent = false;
      in_edge_iter in_edge_it, in_edge_end;

      for (boost::tie(in_edge_it, in_edge_end) = in_edges(curr_vertex, graph_);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        int edge_parid = edge_to_parid[*in_edge_it];
        /* For Load, not mem dependence. */
        /* For GEP, not the dependence that is caused by index. */
        /* For store, not the dependence caused by value or mem dependence. */
        if ((node_microop == LLVM_IR_Load && edge_parid != 1) ||
            (node_microop == LLVM_IR_GetElementPtr && edge_parid != 1) ||
            (node_microop == LLVM_IR_Store && edge_parid != 2))
          continue;

        unsigned parent_id = vertexToName[source(*in_edge_it, graph_)];
        ExecNode* parent_node = exec_nodes.at(parent_id);
        int parent_microop = parent_node->get_microop();
        if (parent_microop == LLVM_IR_GetElementPtr ||
            parent_microop == LLVM_IR_Load || parent_microop == LLVM_IR_Store) {
          // remove address calculation directly
          DynamicVariable dynvar = parent_node->get_dynamic_variable();
          dynvar = getCallerRegID(dynvar);
          node->set_array_label(dynvar.get_variable()->get_name());
          curr_vertex = source(*in_edge_it, graph_);
          node_microop = parent_microop;
          found_parent = true;
          break;
        } else if (parent_microop == LLVM_IR_Alloca) {
          std::string label = getBaseAddressLabel(parent_id);
          node->set_array_label(label);
          break;
        }
      }
      if (!found_parent)
        break;
    }
  }
#if 0
  // TODO: writing the base addresses can cause simulation to freeze when no
  // partitioning is applied to arrays due to how writeBaseAddress() parses the
  // partition number from an array's label. So this is going to be disabled
  // for the time being, until we find a chance to fix this.
  writeBaseAddress();
#endif
}

void BaseDatapath::initExecutingQueue() {
  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (node->get_num_parents() == 0 && !node->is_isolated())
      executingQueue.push_back(node);
  }
}

int BaseDatapath::shortestDistanceBetweenNodes(unsigned int from,
                                               unsigned int to) {
  std::list<std::pair<unsigned int, unsigned int>> queue;
  queue.push_back({ from, 0 });
  while (queue.size() != 0) {
    unsigned int curr_node = queue.front().first;
    unsigned int curr_dist = queue.front().second;
    out_edge_iter out_edge_it, out_edge_end;
    for (boost::tie(out_edge_it, out_edge_end) =
             out_edges(exec_nodes.at(curr_node)->get_vertex(), graph_);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      if (get(boost::edge_name, graph_, *out_edge_it) != CONTROL_EDGE) {
        int child_id = vertexToName[target(*out_edge_it, graph_)];
        if (child_id == to)
          return curr_dist + 1;
        queue.push_back({ child_id, curr_dist + 1 });
      }
    }
    queue.pop_front();
  }
  return -1;
}

partition_config_t::iterator BaseDatapath::getArrayConfigFromAddr(Addr base_addr) {
  auto part_it = partition_config.begin();
  for (; part_it != partition_config.end(); ++part_it) {
    if (part_it->second.base_addr == base_addr) {
      break;
    }
  }
  // If the array label is not found, abort the simulation.
  std::string array_label = part_it->first;
  if (array_label.empty()) {
    std::cerr << "Unknown address " << base_addr
              << std::endl;
    exit(-1);
  }
  return part_it;
}

SrcTypes::UniqueLabel BaseDatapath::getUniqueLabel(ExecNode* node) {
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

unrolling_config_t::iterator BaseDatapath::getUnrollFactor(ExecNode* node) {
  UniqueLabel unrolling_id = getUniqueLabel(node);
  auto config_it = unrolling_config.find(unrolling_id);
  if (config_it != unrolling_config.end())
    return config_it;
  Label* label = srcManager.insert<Label>(std::to_string(node->get_line_num()));
  // TODO: Line numbers are no longer actually used as a standalone identifier,
  // so don't specify it. That field can be removed in the future.
  return unrolling_config.find(
      UniqueLabel(node->get_static_function(), label));
}

std::vector<unsigned> BaseDatapath::getConnectedNodes(unsigned int node_id) {
  std::vector<unsigned> parentNodes = getParentNodes(node_id);
  std::vector<unsigned> childNodes = getChildNodes(node_id);
  parentNodes.insert(parentNodes.end(), childNodes.begin(), childNodes.end());
  return parentNodes;
}

std::vector<unsigned> BaseDatapath::getParentNodes(unsigned int node_id) {
  in_edge_iter in_edge_it, in_edge_end;
  ExecNode* node = exec_nodes.at(node_id);
  Vertex vertex = node->get_vertex();

  std::vector<unsigned> connectedNodes;
  for (boost::tie(in_edge_it, in_edge_end) = in_edges(vertex, graph_);
       in_edge_it != in_edge_end;
       ++in_edge_it) {
    Edge edge = *in_edge_it;
    Vertex source_vertex = source(edge, graph_);
    connectedNodes.push_back(vertexToName[source_vertex]);
  }
  return connectedNodes;
}

std::vector<unsigned> BaseDatapath::getChildNodes(unsigned int node_id) {
  out_edge_iter out_edge_it, out_edge_end;
  ExecNode* node = exec_nodes.at(node_id);
  Vertex vertex = node->get_vertex();

  std::vector<unsigned> connectedNodes;
  for (boost::tie(out_edge_it, out_edge_end) = out_edges(vertex, graph_);
       out_edge_it != out_edge_end;
       ++out_edge_it) {
    Edge edge = *out_edge_it;
    Vertex target_vertex = target(edge, graph_);
    connectedNodes.push_back(vertexToName[target_vertex]);
  }
  return connectedNodes;
}

ExecNode* BaseDatapath::getNextNode(unsigned node_id) {
  auto it = exec_nodes.find(node_id);
  assert(it != exec_nodes.end());
  auto next_it = std::next(it);
  if (next_it == exec_nodes.end())
    return nullptr;
  return next_it->second;
}

// readConfigs
void BaseDatapath::parse_config(std::string& bench,
                                std::string& config_file_name) {
  std::ifstream config_file;
  config_file.open(config_file_name);
  std::string wholeline;
  pipelining = false;
  ready_mode = false;
  num_ports = 1;

  while (!config_file.eof()) {
    wholeline.clear();
    std::getline(config_file, wholeline);
    if (wholeline.size() == 0)  // Empty line.
      continue;
    if (wholeline[0] == '#')  // Comment.
      continue;
    std::string type, rest_line;
    int pos_end_tag = wholeline.find(",");
    if (pos_end_tag == -1)
      break;
    type = wholeline.substr(0, pos_end_tag);
    rest_line = wholeline.substr(pos_end_tag + 1);
    if (!type.compare("flatten")) {
      char function_name[256], label_or_line_num[64];
      sscanf(
          rest_line.c_str(), "%[^,],%[^,]\n", function_name, label_or_line_num);
      Function* function = srcManager.insert<Function>(function_name);
      Label* label = srcManager.insert<Label>(label_or_line_num);
      UniqueLabel unrolling_id(function, label);
      unrolling_config[unrolling_id] = 0;
    } else if (!type.compare("unrolling")) {
      char function_name[256], label_or_line_num[64];
      int factor;
      sscanf(rest_line.c_str(), "%[^,],%[^,],%d\n", function_name,
             label_or_line_num, &factor);
      Function* function = srcManager.insert<Function>(function_name);
      Label* label = srcManager.insert<Label>(label_or_line_num);
      UniqueLabel unrolling_id(function, label);
      unrolling_config[unrolling_id] = factor;
    } else if (!type.compare("partition")) {
      unsigned size = 0, p_factor = 0, wordsize = 0;
      char part_type[256];
      char array_label[256];
      PartitionType p_type;
      MemoryType m_type;
      if (wholeline.find("complete") == std::string::npos) {
        sscanf(rest_line.c_str(),
               "%[^,],%[^,],%d,%d,%d\n",
               part_type,
               array_label,
               &size,
               &wordsize,
               &p_factor);
        m_type = spad;
        if (strncmp(part_type, "cyclic", 6) == 0)
          p_type = cyclic;
        else
          p_type = block;
      } else {
        sscanf(rest_line.c_str(),
               "%[^,],%[^,],%d\n",
               part_type,
               array_label,
               &size);
        p_type = complete;
        m_type = reg;
      }
      long long int addr = 0;
      partition_config[array_label] = {
        m_type, p_type, size, wordsize, p_factor, addr
      };
    } else if (!type.compare("cache")) {
      unsigned size = 0, p_factor = 0, wordsize = 0;
      char array_label[256];
      sscanf(rest_line.c_str(), "%[^,],%d\n", array_label, &size);
      std::string p_type(type);
      long long int addr = 0;
      partition_config[array_label] = {
        cache, none, size, wordsize, p_factor, addr
      };
    } else if (!type.compare("pipelining")) {
      pipelining = atoi(rest_line.c_str());
    } else if (!type.compare("pipeline")) {
      char function_name[256], label_or_line_num[64];
      sscanf(rest_line.c_str(), "%[^,],%[^,]\n", function_name,
             label_or_line_num);
      Function* function = srcManager.insert<Function>(function_name);
      Label* label = srcManager.insert<Label>(label_or_line_num);
      UniqueLabel pipeline_id(function, label);
      if (pipeline_config.find(pipeline_id) == pipeline_config.end()) {
        pipeline_config.insert(pipeline_id);
      } else {
        std::cerr << "Duplicate pipeline directive for " << function_name << "/"
                  << label_or_line_num << ", ignoring.\n";
      }
    } else if (!type.compare("cycle_time")) {
      // Update the global cycle time parameter.
      cycleTime = stof(rest_line);
    } else if (!type.compare("ready_mode")) {
      ready_mode = atoi(rest_line.c_str());
    } else if (!type.compare("scratchpad_ports")) {
      num_ports = atoi(rest_line.c_str());
    } else {
      std::cerr << "Invalid config type: " << wholeline << std::endl;
      exit(1);
    }
  }
  config_file.close();
}

#ifdef USE_DB
void BaseDatapath::setExperimentParameters(std::string experiment_name) {
  use_db = true;
  this->experiment_name = experiment_name;
}

void BaseDatapath::getCommonConfigParameters(int& unrolling_factor,
                                             bool& pipelining_factor,
                                             int& partition_factor) {
  // First, collect pipelining, unrolling, and partitioning parameters. We'll
  // assume that all parameters are uniform for all loops, and we'll insert a
  // path to the actual config file if we need to look up the actual
  // configurations.
  unrolling_factor =
      unrolling_config.empty() ? 1 : unrolling_config.begin()->second;
  pipelining_factor = pipelining;
  partition_factor = partition_config.empty()
                         ? 1
                         : partition_config.begin()->second.part_factor;
}

/* Returns the experiment_id for the experiment_name. If the experiment_name
 * does not exist in the database, then a new experiment_id is created and
 * inserted into the database. */
int BaseDatapath::getExperimentId(sql::Connection* con) {
  sql::ResultSet* res;
  sql::Statement* stmt = con->createStatement();
  stringstream query;
  query << "select id from experiments where strcmp(name, \"" << experiment_name
        << "\") = 0";
  res = stmt->executeQuery(query.str());
  int experiment_id;
  if (res && res->next()) {
    experiment_id = res->getInt(1);
    delete stmt;
    delete res;
  } else {
    // Get the next highest experiment id and insert it into the database.
    query.str("");  // Clear stringstream.
    stmt = con->createStatement();
    res = stmt->executeQuery("select max(id) from experiments");
    if (res && res->next())
      experiment_id = res->getInt(1) + 1;
    else
      experiment_id = 1;
    delete res;
    delete stmt;
    stmt = con->createStatement();
    // TODO: Somehow (elegantly) add support for an experiment description.
    query << "insert into experiments (id, name) values (" << experiment_id
          << ",\"" << experiment_name << "\")";
    stmt->execute(query.str());
    delete stmt;
  }
  return experiment_id;
}

int BaseDatapath::getLastInsertId(sql::Connection* con) {
  sql::Statement* stmt = con->createStatement();
  sql::ResultSet* res = stmt->executeQuery("select last_insert_id()");
  int new_config_id = -1;
  if (res && res->next()) {
    new_config_id = res->getInt(1);
  } else {
    std::cerr << "An unknown error occurred retrieving the config id."
              << std::endl;
  }
  delete stmt;
  delete res;
  return new_config_id;
}

void BaseDatapath::writeSummaryToDatabase(summary_data_t& summary) {
  sql::Driver* driver;
  sql::Connection* con;
  sql::Statement* stmt;
  driver = sql::mysql::get_mysql_driver_instance();
  con = driver->connect(DB_URL, DB_USER, DB_PASS);
  con->setSchema("aladdin");
  con->setAutoCommit(0);  // Begin transaction.
  int config_id = writeConfiguration(con);
  int experiment_id = getExperimentId(con);
  stmt = con->createStatement();
  std::string fullBenchName(benchName);
  std::string benchmark =
      fullBenchName.substr(fullBenchName.find_last_of("/") + 1);
  stringstream query;
  query << "insert into summary (cycles, avg_power, avg_fu_power, "
           "avg_mem_ac, avg_mem_leakage, fu_area, mem_area, "
           "avg_mem_power, total_area, benchmark, experiment_id, config_id) "
           "values (";
  query << summary.num_cycles << "," << summary.avg_power << ","
        << summary.avg_fu_power << "," << summary.avg_mem_dynamic_power << ","
        << summary.mem_leakage_power << "," << summary.fu_area << ","
        << summary.mem_area << "," << summary.avg_mem_power << ","
        << summary.total_area << ","
        << "\"" << benchmark << "\"," << experiment_id << "," << config_id
        << ")";
  stmt->execute(query.str());
  con->commit();  // End transaction.
  delete stmt;
  delete con;
}
#endif
