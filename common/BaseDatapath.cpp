#include <sstream>
#include <boost/tokenizer.hpp>

#include "opcode_func.h"
#include "BaseDatapath.h"
#include "ExecNode.h"

#include "DatabaseDeps.h"

BaseDatapath::BaseDatapath(std::string bench,
                           std::string trace_file,
                           std::string config_file) {
  benchName = (char*)bench.c_str();
  this->trace_file = trace_file;
  this->config_file = config_file;
  use_db = false;
  DDDG* dddg;
  dddg = new DDDG(this, trace_file);
  /* Build initial DDDG. */
  if (dddg->build_initial_dddg()) {
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << "       Aladdin Ends..          " << std::endl;
    std::cerr << "-------------------------------" << std::endl;
    exit(0);
  }
  delete dddg;
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "    Initializing BaseDatapath      " << std::endl;
  std::cerr << "-------------------------------" << std::endl;
  numTotalNodes = exec_nodes.size();

  BGL_FORALL_VERTICES(v, graph_, Graph) {
    exec_nodes[get(boost::vertex_index, graph_, v)]->set_vertex(v);
  }
  vertexToName = get(boost::vertex_index, graph_);

  parse_config(bench, config_file);

  num_cycles = 0;
}

BaseDatapath::~BaseDatapath() {
  for (int i = 0; i < exec_nodes.size(); i++)
    delete exec_nodes[i];
}

void BaseDatapath::addDddgEdge(unsigned int from,
                               unsigned int to,
                               uint8_t parid) {
  if (from != to)
    add_edge(from, to, EdgeProperty(parid), graph_);
}

ExecNode* BaseDatapath::insertNode(unsigned node_id, uint8_t microop) {
  exec_nodes[node_id] = new ExecNode(node_id, microop);
  return exec_nodes[node_id];
}

// optimizationFunctions
void BaseDatapath::setGlobalGraph() {
  std::cerr << "=============================================" << std::endl;
  std::cerr << "      Optimizing...            " << benchName << std::endl;
  std::cerr << "=============================================" << std::endl;
}

void BaseDatapath::memoryAmbiguation() {
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "      Memory Ambiguation       " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  std::unordered_map<std::string, ExecNode*> last_store;
  std::vector<newEdge> to_add_edges;
  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (!node->is_memory_op() || !node->has_vertex())
      continue;
    in_edge_iter in_edge_it, in_edge_end;
    for (tie(in_edge_it, in_edge_end) = in_edges(node->get_vertex(), graph_);
         in_edge_it != in_edge_end;
         ++in_edge_it) {
      Vertex parent_vertex = source(*in_edge_it, graph_);
      ExecNode* parent_node = getNodeFromVertex(parent_vertex);
      if (parent_node->get_microop() != LLVM_IR_GetElementPtr)
        continue;
      if (!parent_node->is_inductive()) {
        node->set_dynamic_mem_op(true);
        std::string unique_id = node->get_static_node_id();
        auto prev_store_it = last_store.find(unique_id);
        if (prev_store_it != last_store.end())
          to_add_edges.push_back({ prev_store_it->second, node, -1 });
        if (node->is_store_op())
          last_store[unique_id] = node;
        break;
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
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "  Remove PHI and Convert Nodes " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

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
    std::vector<pair<ExecNode*, int>> phi_child;
    out_edge_iter out_edge_it, out_edge_end;
    for (tie(out_edge_it, out_edge_end) = out_edges(node_vertex, graph_);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      checked_phi_nodes.insert(node);
      to_remove_edges.insert(*out_edge_it);
      Vertex v = target(*out_edge_it, graph_);
      phi_child.push_back(
          make_pair(getNodeFromVertex(v), edge_to_parid[*out_edge_it]));
    }
    if (phi_child.size() == 0 || boost::in_degree(node_vertex, graph_) == 0)
      continue;
    if (node->get_microop() == LLVM_IR_PHI) {
      // find its parent: phi node can have multiple children, but can only have
      // one parent.
      assert(boost::in_degree(node_vertex, graph_) == 1);
      in_edge_iter in_edge_it = in_edges(node_vertex, graph_).first;
      Vertex parent_vertex = source(*in_edge_it, graph_);
      ExecNode* parent_node = getNodeFromVertex(parent_vertex);
      while (parent_node->get_microop() == LLVM_IR_PHI) {
        checked_phi_nodes.insert(parent_node);
        assert(parent_node->has_vertex());
        Vertex parent_vertex = parent_node->get_vertex();
        if (boost::in_degree(parent_vertex, graph_) == 0)
          break;
        to_remove_edges.insert(*in_edge_it);
        in_edge_it = in_edges(parent_vertex, graph_).first;
        parent_vertex = source(*in_edge_it, graph_);
        parent_node = getNodeFromVertex(parent_vertex);
      }
      to_remove_edges.insert(*in_edge_it);
      for (auto child_it = phi_child.begin(), chil_E = phi_child.end();
           child_it != chil_E;
           ++child_it) {
        to_add_edges.push_back(
            { parent_node, child_it->first, child_it->second });
      }
    } else {
      // convert nodes
      in_edge_iter in_edge_it, in_edge_end;
      for (tie(in_edge_it, in_edge_end) = in_edges(node_vertex, graph_);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        Vertex parent_vertex = source(*in_edge_it, graph_);
        ExecNode* parent_node = getNodeFromVertex(parent_vertex);
        to_remove_edges.insert(*in_edge_it);
        for (auto child_it = phi_child.begin(), chil_E = phi_child.end();
             child_it != chil_E;
             ++child_it) {
          to_add_edges.push_back(
              { parent_node, child_it->first, child_it->second });
        }
      }
    }
    std::vector<pair<ExecNode*, int>>().swap(phi_child);
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
  std::unordered_set<int> flatten_config;
  if (!readFlattenConfig(flatten_config))
    return;
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "         Loop Flatten          " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  std::vector<unsigned> to_remove_nodes;
  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (flatten_config.find(node->get_line_num()) == flatten_config.end())
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
  std::vector<int> num_of_children(numTotalNodes, 0);
  std::vector<unsigned> to_remove_nodes;

  std::vector<Vertex> topo_nodes;
  boost::topological_sort(graph_, std::back_inserter(topo_nodes));
  // bottom nodes first
  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi) {
    Vertex node_vertex = *vi;
    if (boost::degree(node_vertex, graph_) == 0)
      continue;
    unsigned node_id = vertexToName[node_vertex];
    ExecNode* node = exec_nodes[node_id];
    int node_microop = node->get_microop();
    if (num_of_children.at(node_id) == boost::out_degree(node_vertex, graph_) &&
        node_microop != LLVM_IR_SilentStore && node_microop != LLVM_IR_Store &&
        node_microop != LLVM_IR_Ret && !node->is_branch_op()) {
      to_remove_nodes.push_back(node_id);
      // iterate its parents
      in_edge_iter in_edge_it, in_edge_end;
      for (tie(in_edge_it, in_edge_end) = in_edges(node_vertex, graph_);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        int parent_id = vertexToName[source(*in_edge_it, graph_)];
        num_of_children.at(parent_id)++;
      }
    } else if (node->is_branch_op()) {
      // iterate its parents
      in_edge_iter in_edge_it, in_edge_end;
      for (tie(in_edge_it, in_edge_end) = in_edges(node_vertex, graph_);
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
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "  Remove Induction Dependence  " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);
  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    // Revert the inductive flag to false. This guarantees that no child node
    // of this node can mistakenly think its parent is inductive.
    node->set_inductive(false);
    if (!node->has_vertex() || node->is_memory_op())
      continue;
    std::string node_instid = node->get_inst_id();
    if (node_instid.find("indvars") != std::string::npos) {
      node->set_inductive(true);
      if (node->is_add_op())
        node->set_microop(LLVM_IR_IndexAdd);
    } else {
      bool inductive_parents = true;
      in_edge_iter in_edge_it, in_edge_end;
      for (tie(in_edge_it, in_edge_end) = in_edges(node->get_vertex(), graph_);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        if (edge_to_parid[*in_edge_it] == CONTROL_EDGE)
          continue;
        Vertex parent_vertex = source(*in_edge_it, graph_);
        ExecNode* parent_node = getNodeFromVertex(parent_vertex);
        if (!parent_node->is_inductive()) {
          inductive_parents = false;
          break;
        }
      }
      if (inductive_parents) {
        node->set_inductive(true);
        if (node->is_add_op())
          node->set_microop(LLVM_IR_IndexAdd);
      }
    }
  }
}

// called in the end of the whole flow
void BaseDatapath::dumpStats() {
  rescheduleNodesWhenNeeded();
  updateRegStats();
#ifdef DEBUG
  dumpGraph(benchName);
  writeOtherStats();
#endif
}

void BaseDatapath::loopPipelining() {
  if (!readPipeliningConfig()) {
    std::cerr << "Loop Pipelining is not ON." << std::endl;
    return;
  }

  std::unordered_map<int, int> unrolling_config;
  if (!readUnrollingConfig(unrolling_config)) {
    std::cerr << "Loop Unrolling is not defined. " << std::endl;
    std::cerr << "Loop pipelining is only applied to unrolled loops."
              << std::endl;
    return;
  }

  if (loopBound.size() <= 2)
    return;
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "         Loop Pipelining        " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  vertex_iter vi, vi_end;
  std::set<Edge> to_remove_edges;
  std::vector<newEdge> to_add_edges;

  // After loop unrolling, we define strict control dependences between basic
  // block, where all the instructions in the following basic block depend on
  // the prev branch instruction To support loop pipelining, which allows the
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
  ExecNode* curr_node = exec_nodes[*bound_it];
  bound_it++;  // skip first region
  while (node_it != exec_nodes.end()) {
    assert(exec_nodes[*bound_it]->is_branch_op());
    while (node_it->first < *bound_it && node_it != exec_nodes.end()) {
      curr_node = node_it->second;
      if (!curr_node->has_vertex() ||
          boost::degree(curr_node->get_vertex(), graph_) == 0 ||
          curr_node->is_branch_op()) {
        ++node_it;
        continue;
      } else {
        first_non_isolated_node[*bound_it] = curr_node->get_node_id();
        node_it = exec_nodes.find(*bound_it);
        assert(node_it != exec_nodes.end());
        break;
      }
    }
    if (first_non_isolated_node.find(*bound_it) ==
        first_non_isolated_node.end())
      first_non_isolated_node[*bound_it] = *bound_it;
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
    ExecNode* br_node = exec_nodes[first_it->first];
    ExecNode* first_node = exec_nodes[first_it->second];
    if (br_node->is_call_op()) {
      prev_branch_n = nullptr;
      continue;
    }
    if (prev_branch_n != nullptr) {
      // adding dependence between prev_first and first_id
      if (!doesEdgeExist(prev_first_n, first_node))
        to_add_edges.push_back({ prev_first_n, first_node, CONTROL_EDGE });
      // adding dependence between first_id and prev_branch's children
      assert(prev_branch_n->has_vertex());
      out_edge_iter out_edge_it, out_edge_end;
      for (tie(out_edge_it, out_edge_end) =
               out_edges(prev_branch_n->get_vertex(), graph_);
           out_edge_it != out_edge_end;
           ++out_edge_it) {
        Vertex child_vertex = target(*out_edge_it, graph_);
        ExecNode* child_node = getNodeFromVertex(child_vertex);
        if (*child_node < *first_node ||
            edge_to_parid[*out_edge_it] != CONTROL_EDGE)
          continue;
        if (!doesEdgeExist(first_node, child_node))
          to_add_edges.push_back({ first_node, child_node, 1 });
      }
    }
    // update first_id's parents, dependence become strict control dependence
    assert(first_node->has_vertex());
    in_edge_iter in_edge_it, in_edge_end;
    for (tie(in_edge_it, in_edge_end) =
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
    // remove control dependence between br node to its children
    assert(br_node->has_vertex());
    out_edge_iter out_edge_it, out_edge_end;
    for (tie(out_edge_it, out_edge_end) =
             out_edges(br_node->get_vertex(), graph_);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      if (exec_nodes[vertexToName[target(*out_edge_it, graph_)]]->is_call_op())
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
/*
 * Read: graph_, lineNum.gz, unrollingConfig
 * Modify: graph_
 * Write: loop_bound
 */
void BaseDatapath::loopUnrolling() {
  std::unordered_map<int, int> unrolling_config;
  readUnrollingConfig(unrolling_config);

  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "         Loop Unrolling        " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  std::vector<unsigned> to_remove_nodes;
  std::unordered_map<std::string, unsigned> inst_dynamic_counts;
  std::vector<ExecNode*> nodes_between;
  std::vector<newEdge> to_add_edges;

  bool first = false;
  int iter_counts = 0;
  ExecNode* prev_branch = nullptr;

  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (!node->has_vertex())
      continue;
    Vertex node_vertex = node->get_vertex();
    if (boost::degree(node_vertex, graph_) == 0 && !node->is_call_op())
      continue;
    unsigned node_id = node->get_node_id();
    if (!first) {
      first = true;
      loopBound.push_back(node_id);
      prev_branch = node;
    }
    assert(prev_branch != nullptr);
    if (prev_branch != node &&
        !(prev_branch->is_dma_op() && node->is_dma_op())) {
      to_add_edges.push_back({ prev_branch, node, CONTROL_EDGE });
    }
    if (!node->is_branch_op()) {
      nodes_between.push_back(node);
    } else {
      // for the case that the first non-isolated node is also a call node;
      if (node->is_call_op() && *loopBound.rbegin() != node_id) {
        loopBound.push_back(node_id);
        prev_branch = node;
      }

      auto unroll_it = unrolling_config.find(node->get_line_num());
      if (unroll_it == unrolling_config.end()) {  // not unrolling branch
        if (!node->is_call_op()) {
          nodes_between.push_back(node);
          continue;
        }
        // Enforce dependences between branch nodes, including call nodes
        // Except for the case that both two branches are DMA operations.
        // (Two DMA operations can go in parallel.)
        if (!doesEdgeExist(prev_branch, node) &&
            !(prev_branch->is_dma_op() && node->is_dma_op())) {
          to_add_edges.push_back({ prev_branch, node, CONTROL_EDGE });
        }
        for (auto prev_node_it = nodes_between.begin(), E = nodes_between.end();
             prev_node_it != E;
             prev_node_it++) {
          if (!doesEdgeExist(*prev_node_it, node) &&
              !((*prev_node_it)->is_dma_op() && node->is_dma_op())) {
            to_add_edges.push_back({ *prev_node_it, node, CONTROL_EDGE });
          }
        }
        nodes_between.clear();
        nodes_between.push_back(node);
        prev_branch = node;
      } else {  // unrolling the branch
        // Counting number of loop iterations.
        int factor = unroll_it->second;
        char unique_inst_id[256];
        sprintf(
            unique_inst_id, "%d-%d", node->get_microop(), node->get_line_num());
        auto it = inst_dynamic_counts.find(unique_inst_id);
        if (it == inst_dynamic_counts.end()) {
          inst_dynamic_counts[unique_inst_id] = 1;
          it = inst_dynamic_counts.find(unique_inst_id);
        } else {
          it->second++;
        }
        if (it->second % factor == 0) {
          loopBound.push_back(node_id);
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
        } else
          to_remove_nodes.push_back(node_id);
      }
    }
  }
  loopBound.push_back(numTotalNodes);

  if (iter_counts == 0 && unrolling_config.size() != 0) {
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << "Loop Unrolling Factor is Larger than the Loop Trip Count."
              << std::endl;
    std::cerr << "Loop Unrolling is NOT applied. Please choose a smaller "
              << "unrolling factor." << std::endl;
    std::cerr << "-------------------------------" << std::endl;
  }
  updateGraphWithNewEdges(to_add_edges);
  updateGraphWithIsolatedNodes(to_remove_nodes);
  cleanLeafNodes();
}
/*
 * Read: loop_bound, flattenConfig, graph, actualAddress
 * Modify: graph_
 */
void BaseDatapath::removeSharedLoads() {

  std::unordered_set<int> flatten_config;
  if (!readFlattenConfig(flatten_config) && loopBound.size() <= 2)
    return;
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "          Load Buffer          " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  vertex_iter vi, vi_end;

  std::set<Edge> to_remove_edges;
  std::vector<newEdge> to_add_edges;

  int shared_loads = 0;
  auto bound_it = loopBound.begin();

  auto node_it = exec_nodes.begin();
  while (node_it != exec_nodes.end()) {
    std::unordered_map<unsigned, ExecNode*> address_loaded;
    while (node_it->first < *bound_it && node_it != exec_nodes.end()) {
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
          for (tie(out_edge_it, out_edge_end) = out_edges(load_node, graph_);
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
          for (tie(in_edge_it, in_edge_end) = in_edges(load_node, graph_);
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
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "          Store Buffer         " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  std::vector<newEdge> to_add_edges;
  std::vector<unsigned> to_remove_nodes;

  auto bound_it = loopBound.begin();
  auto node_it = exec_nodes.begin();

  while (node_it != exec_nodes.end()) {
    while (node_it->first < *bound_it && node_it != exec_nodes.end()) {
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
        for (tie(out_edge_it, out_edge_end) = out_edges(node_vertex, graph_);
             out_edge_it != out_edge_end;
             ++out_edge_it) {
          Vertex child_vertex = target(*out_edge_it, graph_);
          ExecNode* child_node = getNodeFromVertex(child_vertex);
          if (child_node->is_load_op()) {
            if (child_node->is_dynamic_mem_op() ||
                child_node->get_node_id() >= (unsigned)*bound_it)
              continue;
            else
              store_child.push_back(child_vertex);
          }
        }

        if (store_child.size() > 0) {
          bool parent_found = false;
          Vertex store_parent;
          in_edge_iter in_edge_it, in_edge_end;
          for (tie(in_edge_it, in_edge_end) = in_edges(node_vertex, graph_);
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
              for (tie(out_edge_it, out_edge_end) =
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
  std::unordered_set<int> flatten_config;
  if (!readFlattenConfig(flatten_config) && loopBound.size() <= 2)
    return;

  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "     Remove Repeated Store     " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  int node_id = numTotalNodes - 1;
  auto bound_it = loopBound.end();
  bound_it--;
  bound_it--;
  while (node_id >= 0) {
    unordered_map<unsigned, int> address_store_map;

    while (node_id >= *bound_it && node_id >= 0) {
      ExecNode* node = exec_nodes[node_id];
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
        if (node->is_dynamic_mem_op()) {
          if (boost::out_degree(node->get_vertex(), graph_) == 0) {
            node->set_microop(LLVM_IR_SilentStore);
          } else {
            int num_of_real_children = 0;
            out_edge_iter out_edge_it, out_edge_end;
            for (tie(out_edge_it, out_edge_end) =
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
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "     Tree Height Reduction     " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  std::vector<bool> updated(numTotalNodes, 0);
  std::vector<int> bound_region(numTotalNodes, 0);

  int region_id = 0;
  unsigned node_id = 0;
  auto bound_it = loopBound.begin();
  while (node_id < *bound_it) {
    bound_region.at(node_id) = region_id;
    node_id++;
    if (node_id == *bound_it) {
      region_id++;
      bound_it++;
      if (bound_it == loopBound.end())
        break;
    }
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
    std::vector<pair<ExecNode*, bool>> leaves;
    std::vector<ExecNode*> associative_chain;
    associative_chain.push_back(node);
    int chain_id = 0;
    while (chain_id < associative_chain.size()) {
      ExecNode* chain_node = associative_chain[chain_id];
      if (chain_node->is_associative()) {
        updated.at(chain_node->get_node_id()) = 1;
        int num_of_chain_parents = 0;
        in_edge_iter in_edge_it, in_edge_end;
        for (tie(in_edge_it, in_edge_end) =
                 in_edges(chain_node->get_vertex(), graph_);
             in_edge_it != in_edge_end;
             ++in_edge_it) {
          int parent_id = vertexToName[source(*in_edge_it, graph_)];
          if (exec_nodes[parent_id]->is_branch_op())
            continue;
          num_of_chain_parents++;
        }
        if (num_of_chain_parents == 2) {
          nodes.push_front(chain_node);
          for (tie(in_edge_it, in_edge_end) =
                   in_edges(chain_node->get_vertex(), graph_);
               in_edge_it != in_edge_end;
               ++in_edge_it) {
            Vertex parent_vertex = source(*in_edge_it, graph_);
            int parent_id = vertexToName[parent_vertex];
            ExecNode* parent_node = getNodeFromVertex(parent_vertex);
            assert(*parent_node < *chain_node);
            if (parent_node->is_branch_op())
              continue;

            Edge curr_edge = *in_edge_it;
            tmp_remove_edges.push_back(curr_edge);
            int parent_region = bound_region.at(parent_id);
            if (parent_region == node_region) {
              updated.at(parent_id) = 1;
              if (!parent_node->is_associative())
                leaves.push_back(make_pair(parent_node, 0));
              else {
                out_edge_iter out_edge_it, out_edge_end;
                int num_of_children = 0;
                for (tie(out_edge_it, out_edge_end) =
                         out_edges(parent_vertex, graph_);
                     out_edge_it != out_edge_end;
                     ++out_edge_it) {
                  if (edge_to_parid[*out_edge_it] != CONTROL_EDGE)
                    num_of_children++;
                }

                if (num_of_children == 1)
                  associative_chain.push_back(parent_node);
                else
                  leaves.push_back(make_pair(parent_node, 0));
              }
            } else {
              leaves.push_back(make_pair(parent_node, 1));
            }
          }
        } else {
          leaves.push_back(make_pair(chain_node, 0));
        }
      } else {
        leaves.push_back(make_pair(chain_node, 0));
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
      if (leaf_it->second == 0)
        rank_map[leaf_it->first] = 0;
      else
        rank_map[leaf_it->first] = numTotalNodes;
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
      assert((node1->get_node_id() != numTotalNodes) &&
             (node2->get_node_id() != numTotalNodes));
      to_add_edges.push_back({ node1, *new_node_it, 1 });
      to_add_edges.push_back({ node2, *new_node_it, 1 });

      // place the new node in the map, remove the two old nodes
      rank_map[*new_node_it] = max(rank_map[node1], rank_map[node2]) + 1;
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
  unsigned min_rank = numTotalNodes;
  for (auto it = rank_map.begin(); it != rank_map.end(); ++it) {
    int node_rank = it->second;
    if (node_rank < min_rank) {
      *node1 = it->first;
      min_rank = node_rank;
    }
  }
  min_rank = numTotalNodes;
  for (auto it = rank_map.begin(); it != rank_map.end(); ++it) {
    int node_rank = it->second;
    if ((it->first != *node1) && (node_rank < min_rank)) {
      *node2 = it->first;
      min_rank = node_rank;
    }
  }
}

void BaseDatapath::updateGraphWithNewEdges(std::vector<newEdge>& to_add_edges) {
  for (auto it = to_add_edges.begin(); it != to_add_edges.end(); ++it) {
    if (*it->from != *it->to && !doesEdgeExist(it->from, it->to))
      get(boost::edge_name, graph_)[add_edge(
          it->from->get_node_id(), it->to->get_node_id(), graph_).first] =
          it->parid;
  }
}
void BaseDatapath::updateGraphWithIsolatedNodes(
    std::vector<unsigned>& to_remove_nodes) {
  for (auto it = to_remove_nodes.begin(); it != to_remove_nodes.end(); ++it)
    clear_vertex(exec_nodes[*it]->get_vertex(), graph_);
}

void BaseDatapath::updateGraphWithIsolatedEdges(
    std::set<Edge>& to_remove_edges) {
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
  std::string bn(benchName);

  activity_map mul_activity, add_activity;
  activity_map bit_activity;
  activity_map shifter_activity;
  activity_map ld_activity, st_activity;

  max_activity_map max_mul_per_function;
  max_activity_map max_add_per_function;
  max_activity_map max_bit_per_function;
  max_activity_map max_shifter_per_function;

  std::vector<std::string> comp_partition_names;
  std::vector<std::string> mem_partition_names;
  registers.getRegisterNames(comp_partition_names);
  getMemoryBlocks(mem_partition_names);

  initPerCycleActivity(comp_partition_names,
                       mem_partition_names,
                       ld_activity,
                       st_activity,
                       mul_activity,
                       add_activity,
                       bit_activity,
                       shifter_activity,
                       max_mul_per_function,
                       max_add_per_function,
                       max_bit_per_function,
                       max_shifter_per_function,
                       num_cycles);

  updatePerCycleActivity(ld_activity,
                         st_activity,
                         mul_activity,
                         add_activity,
                         bit_activity,
                         shifter_activity,
                         max_mul_per_function,
                         max_add_per_function,
                         max_bit_per_function,
                         max_shifter_per_function);

  outputPerCycleActivity(comp_partition_names,
                         mem_partition_names,
                         ld_activity,
                         st_activity,
                         mul_activity,
                         add_activity,
                         bit_activity,
                         shifter_activity,
                         max_mul_per_function,
                         max_add_per_function,
                         max_bit_per_function,
                         max_shifter_per_function);
}

void BaseDatapath::initPerCycleActivity(
    std::vector<std::string>& comp_partition_names,
    std::vector<std::string>& mem_partition_names,
    activity_map& ld_activity,
    activity_map& st_activity,
    activity_map& mul_activity,
    activity_map& add_activity,
    activity_map& bit_activity,
    activity_map& shifter_activity,
    max_activity_map& max_mul_per_function,
    max_activity_map& max_add_per_function,
    max_activity_map& max_bit_per_function,
    max_activity_map& max_shifter_per_function,
    int num_cycles) {
  for (auto it = comp_partition_names.begin(); it != comp_partition_names.end();
       ++it) {
    ld_activity.insert({ *it, make_vector(num_cycles) });
    st_activity.insert({ *it, make_vector(num_cycles) });
  }
  for (auto it = mem_partition_names.begin(); it != mem_partition_names.end();
       ++it) {
    ld_activity.insert({ *it, make_vector(num_cycles) });
    st_activity.insert({ *it, make_vector(num_cycles) });
  }
  for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
    mul_activity.insert({ *it, make_vector(num_cycles) });
    add_activity.insert({ *it, make_vector(num_cycles) });
    bit_activity.insert({ *it, make_vector(num_cycles) });
    shifter_activity.insert({ *it, make_vector(num_cycles) });
    max_mul_per_function.insert({ *it, 0 });
    max_add_per_function.insert({ *it, 0 });
    max_bit_per_function.insert({ *it, 0 });
    max_shifter_per_function.insert({ *it, 0 });
  }
}

void BaseDatapath::updatePerCycleActivity(
    activity_map& ld_activity,
    activity_map& st_activity,
    activity_map& mul_activity,
    activity_map& add_activity,
    activity_map& bit_activity,
    activity_map& shifter_activity,
    max_activity_map& max_mul_per_function,
    max_activity_map& max_add_per_function,
    max_activity_map& max_bit_per_function,
    max_activity_map& max_shifter_per_function) {
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
    std::string func_id = node->get_static_method();

    if (node->get_node_id() == *bound_it) {
      if (max_add_per_function[func_id] < num_adds_so_far)
        max_add_per_function[func_id] = num_adds_so_far;
      if (max_bit_per_function[func_id] < num_bits_so_far)
        max_bit_per_function[func_id] = num_bits_so_far;
      if (max_shifter_per_function[func_id] < num_shifters_so_far)
        max_shifter_per_function[func_id] = num_shifters_so_far;
      num_adds_so_far = 0;
      num_bits_so_far = 0;
      num_shifters_so_far = 0;
      bound_it++;
    }
    if (node->is_isolated())
      continue;
    int node_level = node->get_execution_cycle();
    int node_microop = node->get_microop();

    if (is_mul_op(node_microop))
      mul_activity[func_id].at(node_level) += 1;
    else if (is_add_op(node_microop)) {
      add_activity[func_id].at(node_level) += 1;
      num_adds_so_far += 1;
    } else if (is_shifter_op(node_microop)) {
      shifter_activity[func_id].at(node_level) += 1;
      num_shifters_so_far += 1;
    } else if (is_bit_op(node_microop)) {
      bit_activity[func_id].at(node_level) += 1;
      num_bits_so_far += 1;
    } else if (is_load_op(node_microop)) {
      std::string base_addr = node->get_array_label();
      if (ld_activity.find(base_addr) != ld_activity.end())
        ld_activity[base_addr].at(node_level) += 1;
    } else if (is_store_op(node_microop)) {
      std::string base_addr = node->get_array_label();
      if (st_activity.find(base_addr) != st_activity.end())
        st_activity[base_addr].at(node_level) += 1;
    }
  }
  for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
    max_mul_per_function[*it] =
        *(std::max_element(mul_activity[*it].begin(), mul_activity[*it].end()));
  }
}

void BaseDatapath::outputPerCycleActivity(
    std::vector<std::string>& comp_partition_names,
    std::vector<std::string>& mem_partition_names,
    activity_map& ld_activity,
    activity_map& st_activity,
    activity_map& mul_activity,
    activity_map& add_activity,
    activity_map& bit_activity,
    activity_map& shifter_activity,
    max_activity_map& max_mul_per_function,
    max_activity_map& max_add_per_function,
    max_activity_map& max_bit_per_function,
    max_activity_map& max_shifter_per_function) {
  /*Set the constants*/
  float add_int_power, add_switch_power, add_leak_power, add_area;
  float mul_int_power, mul_switch_power, mul_leak_power, mul_area;
  float reg_int_power_per_bit, reg_switch_power_per_bit;
  float reg_leak_power_per_bit, reg_area_per_bit;
  float bit_int_power, bit_switch_power, bit_leak_power, bit_area;
  float shifter_int_power, shifter_switch_power, shifter_leak_power,
      shifter_area;

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

  ofstream stats, power_stats;
  std::string bn(benchName);
  std::string file_name = bn + "_stats";
  stats.open(file_name.c_str());
  file_name += "_power";
  power_stats.open(file_name.c_str());

  stats << "cycles," << num_cycles << "," << numTotalNodes << std::endl;
  power_stats << "cycles," << num_cycles << "," << numTotalNodes << std::endl;
  stats << num_cycles << ",";
  power_stats << num_cycles << ",";

  /*Start writing the second line*/
  for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
    stats << *it << "-mul," << *it << "-add," << *it << "-bit," << *it
          << "-shifter,";
    power_stats << *it << "-mul," << *it << "-add," << *it << "-bit," << *it
                << "-shifter,";
  }
  stats << "reg,";
  power_stats << "reg,";
  for (auto it = mem_partition_names.begin(); it != mem_partition_names.end();
       ++it) {
    stats << *it << "-read," << *it << "-write,";
  }
  stats << std::endl;
  power_stats << std::endl;
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
  for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
    max_bit += max_bit_per_function[*it];
    max_add += max_add_per_function[*it];
    max_mul += max_mul_per_function[*it];
    max_shifter += max_shifter_per_function[*it];
  }

  float add_leakage_power = add_leak_power * max_add;
  float mul_leakage_power = mul_leak_power * max_mul;
  float bit_leakage_power = bit_leak_power * max_bit;
  float shifter_leakage_power = shifter_leak_power * max_shifter;
  float reg_leakage_power =
      registers.getTotalLeakagePower() + reg_leak_power_per_bit * 32 * max_reg;
  float fu_leakage_power = mul_leakage_power + add_leakage_power +
                           reg_leakage_power + bit_leakage_power +
                           shifter_leakage_power;
  /*Finish caculating the number of FUs and leakage power*/

  float fu_dynamic_energy = 0;

  /*Start writing per cycle activity */
  for (unsigned curr_level = 0; ((int)curr_level) < num_cycles; ++curr_level) {
    stats << curr_level << ",";
    power_stats << curr_level << ",";
    // For FUs
    for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
      float curr_mul_dynamic_power =
          (mul_switch_power + mul_int_power) * mul_activity[*it].at(curr_level);
      float curr_add_dynamic_power =
          (add_switch_power + add_int_power) * add_activity[*it].at(curr_level);
      float curr_bit_dynamic_power =
          (bit_switch_power + bit_int_power) * bit_activity[*it].at(curr_level);
      float curr_shifter_dynamic_power =
          (shifter_switch_power + shifter_int_power) *
          shifter_activity[*it].at(curr_level);
      fu_dynamic_energy +=
          (curr_mul_dynamic_power + curr_add_dynamic_power +
           curr_bit_dynamic_power + curr_shifter_dynamic_power) *
          cycleTime;

      stats << mul_activity[*it].at(curr_level) << ","
            << add_activity[*it].at(curr_level) << ","
            << bit_activity[*it].at(curr_level) << ","
            << shifter_activity[*it].at(curr_level) << ",";
      power_stats << curr_mul_dynamic_power + mul_leakage_power << ","
                  << curr_add_dynamic_power + add_leakage_power << ","
                  << curr_bit_dynamic_power + bit_leakage_power << ","
                  << curr_shifter_dynamic_power + shifter_leakage_power << ",";
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
      curr_reg_reads += ld_activity.at(*it).at(curr_level);
      curr_reg_writes += st_activity.at(*it).at(curr_level);
      curr_reg_dynamic_energy +=
          registers.getReadEnergy(*it) * ld_activity.at(*it).at(curr_level) +
          registers.getWriteEnergy(*it) * st_activity.at(*it).at(curr_level);
    }
    fu_dynamic_energy += curr_reg_dynamic_energy;

    stats << curr_reg_reads << "," << curr_reg_writes << ",";
    power_stats << curr_reg_dynamic_energy / cycleTime + reg_leakage_power;

    for (auto it = mem_partition_names.begin(); it != mem_partition_names.end();
         ++it)
      stats << ld_activity.at(*it).at(curr_level) << ","
            << st_activity.at(*it).at(curr_level) << ",";
    stats << std::endl;
    power_stats << std::endl;
  }
  stats.close();
  power_stats.close();

  float avg_mem_power = 0, avg_mem_dynamic_power = 0, mem_leakage_power = 0;

  getAverageMemPower(
      num_cycles, &avg_mem_power, &avg_mem_dynamic_power, &mem_leakage_power);

  float avg_fu_dynamic_power = fu_dynamic_energy / (cycleTime * num_cycles);
  float avg_fu_power = avg_fu_dynamic_power + fu_leakage_power;
  float avg_power = avg_fu_power + avg_mem_power;

  float mem_area = getTotalMemArea();
  float fu_area = registers.getTotalArea() + add_area * max_add +
                  mul_area * max_mul + reg_area_per_bit * 32 * max_reg +
                  bit_area * max_bit + shifter_area * max_shifter;
  float total_area = mem_area + fu_area;

  // Summary output.
  summary_data_t summary;
  summary.benchName = benchName;
  summary.num_cycles = num_cycles;
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

  writeSummary(std::cerr, summary);
  ofstream summary_file;
  file_name = bn + "_summary";
  summary_file.open(file_name.c_str());
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
  outfile << "Num of Multipliers (32-bit): " << summary.max_mul << std::endl;
  outfile << "Num of Adders (32-bit): " << summary.max_add << std::endl;
  outfile << "Num of Bit-wise Operators (32-bit): " << summary.max_bit
          << std::endl;
  outfile << "Num of Shifters (32-bit): " << summary.max_shifter << std::endl;
  outfile << "Num of Registers (32-bit): " << summary.max_reg << std::endl;
  outfile << "===============================" << std::endl;
  outfile << "        Aladdin Results        " << std::endl;
  outfile << "===============================" << std::endl;
}

void BaseDatapath::writeBaseAddress() {
  ostringstream file_name;
  file_name << benchName << "_baseAddr.gz";
  gzFile gzip_file;
  gzip_file = gzopen(file_name.str().c_str(), "w");
  for (auto it = exec_nodes.begin(), E = exec_nodes.end(); it != E; ++it) {
    char original_label[256];
    int partition_id;
    std::string partitioned_label = it->second->get_array_label();
    sscanf(
        partitioned_label.c_str(), "%[^-]-%d", original_label, &partition_id);
    gzprintf(gzip_file,
             "node:%u,part:%s,base:%lld\n",
             it->first,
             partitioned_label.c_str(),
             arrayBaseAddress[original_label]);
  }
  gzclose(gzip_file);
}

void BaseDatapath::writeOtherStats() {
  // First collect the data from exec_nodes.
  std::vector<int> microop(numTotalNodes, 0);
  std::vector<int> exec_cycle(numTotalNodes, 0);
  std::vector<bool> isolated(numTotalNodes, false);

  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    unsigned node_id = node_it->first;
    ExecNode* node = node_it->second;
    microop[node_id] = node->get_microop();
    exec_cycle[node_id] = node->get_execution_cycle();
    isolated[node_id] = node->is_isolated();
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
void BaseDatapath::setGraphForStepping() {
  std::cerr << "=============================================" << std::endl;
  std::cerr << "      Scheduling...            " << benchName << std::endl;
  std::cerr << "=============================================" << std::endl;

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
  executingQueue.clear();
  readyToExecuteQueue.clear();
  initExecutingQueue();
}

void BaseDatapath::dumpGraph(std::string graph_name) {
  std::unordered_map<Vertex, unsigned> vertexToMicroop;
  BGL_FORALL_VERTICES(v, graph_, Graph) {
    vertexToMicroop[v] =
        exec_nodes[get(boost::vertex_index, graph_, v)]->get_microop();
  }
  std::ofstream out(graph_name + "_graph.dot");
  write_graphviz(out, graph_, boost::make_label_writer(vertexToMicroop));
}
/*As Late As Possible (ALAP) rescheduling for non-memory, non-control nodes.
  The first pass of scheduling is as earlyl as possible, whenever a node's
  parents are ready, the node is executed. This mode of executing potentially
  can lead to values that are produced way earlier than they are needed. For
  such case, we add an ALAP rescheduling pass to reorganize the graph without
  changing the critical path and memory nodes, but produce a more balanced
  design.*/
int BaseDatapath::rescheduleNodesWhenNeeded() {
  std::vector<Vertex> topo_nodes;
  boost::topological_sort(graph_, std::back_inserter(topo_nodes));
  // bottom nodes first
  std::vector<int> earliest_child(numTotalNodes, num_cycles);
  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi) {
    unsigned node_id = vertexToName[*vi];
    ExecNode* node = exec_nodes[node_id];
    if (node->is_isolated())
      continue;
    if (!node->is_memory_op() && !node->is_branch_op())
      if ((earliest_child.at(node_id) - 1) > node->get_execution_cycle())
        node->set_execution_cycle(earliest_child.at(node_id) - 1);

    in_edge_iter in_i, in_end;
    for (tie(in_i, in_end) = in_edges(*vi, graph_); in_i != in_end; ++in_i) {
      int parent_id = vertexToName[source(*in_i, graph_)];
      if (earliest_child.at(parent_id) > node->get_execution_cycle())
        earliest_child.at(parent_id) = node->get_execution_cycle();
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
    int node_level = node->get_execution_cycle();
    int max_children_level = node_level;

    Vertex node_vertex = node->get_vertex();
    out_edge_iter out_edge_it, out_edge_end;
    std::set<int> children_levels;
    for (tie(out_edge_it, out_edge_end) = out_edges(node_vertex, graph_);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      int child_id = vertexToName[target(*out_edge_it, graph_)];
      ExecNode* child_node = exec_nodes[child_id];
      if (child_node->is_control_op() || child_node->is_load_op())
        continue;

      int child_level = child_node->get_execution_cycle();
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
    return 1;
  return 0;
}

// Marks a node as completed and advances the executing queue iterator.
void BaseDatapath::markNodeCompleted(
    std::list<ExecNode*>::iterator& executingQueuePos, int& advance_to) {
  ExecNode* node = *executingQueuePos;
  executedNodes++;
  node->set_execution_cycle(num_cycles);
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
  for (tie(out_edge_it, out_edge_end) = out_edges(node_vertex, graph_);
       out_edge_it != out_edge_end;
       ++out_edge_it) {
    Vertex child_vertex = target(*out_edge_it, graph_);
    ExecNode* child_node = getNodeFromVertex(child_vertex);
    int edge_parid = edgeToParid[*out_edge_it];
    if (child_node->get_num_parents() > 0) {
      child_node->decr_num_parents();
      if (child_node->get_num_parents() == 0) {
        if ((child_node->node_latency() == 0 || node->node_latency() == 0) &&
            edge_parid != CONTROL_EDGE) {
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
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "       Init Base Address       " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  vertex_iter vi, vi_end;
  for (tie(vi, vi_end) = vertices(graph_); vi != vi_end; ++vi) {
    if (boost::degree(*vi, graph_) == 0)
      continue;
    Vertex curr_vertex = *vi;
    ExecNode* node = getNodeFromVertex(curr_vertex);
    if (!node->is_memory_op())
      continue;
    int node_microop = node->get_microop();
    // iterate its parents, until it finds the root parent
    while (true) {
      bool found_parent = 0;
      in_edge_iter in_edge_it, in_edge_end;

      for (tie(in_edge_it, in_edge_end) = in_edges(curr_vertex, graph_);
           in_edge_it != in_edge_end;
           ++in_edge_it) {
        int edge_parid = edge_to_parid[*in_edge_it];
        /* For Load, not mem dependence.*/
        /* For GEP, not the dependence that is caused by index */
        /* For store, not the dependence caused by value or mem dependence */
        if ((node_microop == LLVM_IR_Load && edge_parid != 1) ||
            (node_microop == LLVM_IR_GetElementPtr && edge_parid != 1) ||
            (node_microop == LLVM_IR_Store && edge_parid != 2))
          continue;

        unsigned parent_id = vertexToName[source(*in_edge_it, graph_)];
        int parent_microop = exec_nodes[parent_id]->get_microop();
        if (parent_microop == LLVM_IR_GetElementPtr ||
            parent_microop == LLVM_IR_Load ||
            parent_microop == LLVM_IR_Store) {
          // remove address calculation directly
          std::string label = getBaseAddressLabel(parent_id);
          node->set_array_label(label);
          curr_vertex = source(*in_edge_it, graph_);
          node_microop = parent_microop;
          found_parent = 1;
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
#ifdef DEBUG
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
  std::list<pair<unsigned int, unsigned int>> queue;
  queue.push_back({ from, 0 });
  while (queue.size() != 0) {
    unsigned int curr_node = queue.front().first;
    unsigned int curr_dist = queue.front().second;
    out_edge_iter out_edge_it, out_edge_end;
    for (tie(out_edge_it, out_edge_end) =
             out_edges(exec_nodes[curr_node]->get_vertex(), graph_);
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

// readConfigs
bool BaseDatapath::readPipeliningConfig() {
  ifstream config_file;
  std::string file_name(benchName);
  file_name += "_pipelining_config";
  config_file.open(file_name.c_str());
  if (!config_file.is_open())
    return 0;
  std::string wholeline;
  getline(config_file, wholeline);
  if (wholeline.size() == 0)
    return 0;
  bool flag = atoi(wholeline.c_str());
  return flag;
}

bool BaseDatapath::readUnrollingConfig(
    std::unordered_map<int, int>& unrolling_config) {
  ifstream config_file;
  std::string file_name(benchName);
  file_name += "_unrolling_config";
  config_file.open(file_name.c_str());
  if (!config_file.is_open())
    return 0;
  while (!config_file.eof()) {
    std::string wholeline;
    getline(config_file, wholeline);
    if (wholeline.size() == 0)
      break;
    char func[256];
    int line_num, factor;
    sscanf(wholeline.c_str(), "%[^,],%d,%d\n", func, &line_num, &factor);
    unrolling_config[line_num] = factor;
  }
  config_file.close();
  return 1;
}

bool BaseDatapath::readFlattenConfig(std::unordered_set<int>& flatten_config) {
  ifstream config_file;
  std::string file_name(benchName);
  file_name += "_flatten_config";
  config_file.open(file_name.c_str());
  if (!config_file.is_open())
    return 0;
  while (!config_file.eof()) {
    std::string wholeline;
    getline(config_file, wholeline);
    if (wholeline.size() == 0)
      break;
    char func[256];
    int line_num;
    sscanf(wholeline.c_str(), "%[^,],%d\n", func, &line_num);
    flatten_config.insert(line_num);
  }
  config_file.close();
  return 1;
}

bool BaseDatapath::readCompletePartitionConfig(
    std::unordered_map<std::string, unsigned>& config) {
  std::string comp_partition_file(benchName);
  comp_partition_file += "_complete_partition_config";

  if (!fileExists(comp_partition_file))
    return 0;

  ifstream config_file;
  config_file.open(comp_partition_file);
  std::string wholeline;
  while (!config_file.eof()) {
    getline(config_file, wholeline);
    if (wholeline.size() == 0)
      break;
    unsigned size;
    char type[256];
    char base_addr[256];
    sscanf(wholeline.c_str(), "%[^,],%[^,],%d\n", type, base_addr, &size);
    config[base_addr] = size;
  }
  config_file.close();
  return 1;
}

bool BaseDatapath::readPartitionConfig(
    std::unordered_map<std::string, partitionEntry>& partition_config) {
  ifstream config_file;
  std::string file_name(benchName);
  file_name += "_partition_config";
  if (!fileExists(file_name))
    return 0;

  config_file.open(file_name.c_str());
  std::string wholeline;
  while (!config_file.eof()) {
    getline(config_file, wholeline);
    if (wholeline.size() == 0)
      break;
    unsigned size, p_factor, wordsize;
    char type[256];
    char base_addr[256];
    sscanf(wholeline.c_str(),
           "%[^,],%[^,],%d,%d,%d\n",
           type,
           base_addr,
           &size,
           &wordsize,
           &p_factor);
    std::string p_type(type);
    partition_config[base_addr] = { p_type, size, wordsize, p_factor };
  }
  config_file.close();
  return 1;
}
void BaseDatapath::parse_config(std::string bench,
                                std::string config_file_name) {
  ifstream config_file;
  config_file.open(config_file_name);
  std::string wholeline;

  std::vector<std::string> flatten_config;
  std::vector<std::string> unrolling_config;
  std::vector<std::string> partition_config;
  std::vector<std::string> comp_partition_config;
  std::vector<std::string> pipelining_config;

  while (!config_file.eof()) {
    wholeline.clear();
    getline(config_file, wholeline);
    if (wholeline.size() == 0)
      break;
    string type, rest_line;
    int pos_end_tag = wholeline.find(",");
    if (pos_end_tag == -1)
      break;
    type = wholeline.substr(0, pos_end_tag);
    rest_line = wholeline.substr(pos_end_tag + 1);
    if (!type.compare("flatten")) {
      flatten_config.push_back(rest_line);
    } else if (!type.compare("unrolling")) {
      unrolling_config.push_back(rest_line);
    } else if (!type.compare("partition")) {
      if (wholeline.find("complete") == std::string::npos)
        partition_config.push_back(rest_line);
      else
        comp_partition_config.push_back(rest_line);
    } else if (!type.compare("pipelining")) {
      pipelining_config.push_back(rest_line);
    } else if (!type.compare("cycle_time")) {
      // Update the global cycle time parameter.
      cycleTime = stof(rest_line);
    } else {
      cerr << "what else? " << wholeline << endl;
      exit(0);
    }
  }
  config_file.close();
  if (flatten_config.size() != 0) {
    string file_name(bench);
    file_name += "_flatten_config";
    ofstream output;
    output.open(file_name);
    for (unsigned i = 0; i < flatten_config.size(); ++i)
      output << flatten_config.at(i) << endl;
    output.close();
  }
  if (unrolling_config.size() != 0) {
    string file_name(bench);
    file_name += "_unrolling_config";
    ofstream output;
    output.open(file_name);
    for (unsigned i = 0; i < unrolling_config.size(); ++i)
      output << unrolling_config.at(i) << endl;
    output.close();
  }
  if (pipelining_config.size() != 0) {
    string pipelining(bench);
    pipelining += "_pipelining_config";

    ofstream pipe_config;
    pipe_config.open(pipelining);
    for (unsigned i = 0; i < pipelining_config.size(); ++i)
      pipe_config << pipelining_config.at(i) << endl;
    pipe_config.close();
  }
  if (partition_config.size() != 0) {
    string partition(bench);
    partition += "_partition_config";

    ofstream part_config;
    part_config.open(partition);
    for (unsigned i = 0; i < partition_config.size(); ++i)
      part_config << partition_config.at(i) << endl;
    part_config.close();
  }
  if (comp_partition_config.size() != 0) {
    string complete_partition(bench);
    complete_partition += "_complete_partition_config";

    ofstream comp_config;
    comp_config.open(complete_partition);
    for (unsigned i = 0; i < comp_partition_config.size(); ++i)
      comp_config << comp_partition_config.at(i) << endl;
    comp_config.close();
  }
}

/* Tokenizes an input string and returns a vector. */
void BaseDatapath::tokenizeString(std::string input,
                                  std::vector<int>& tokenized_list) {
  using namespace boost;
  tokenizer<> tok(input);
  for (tokenizer<>::iterator beg = tok.begin(); beg != tok.end(); ++beg) {
    int value;
    istringstream(*beg) >> value;
    tokenized_list.push_back(value);
  }
}

#ifdef USE_DB
void BaseDatapath::setExperimentParameters(std::string experiment_name) {
  use_db = true;
  this->experiment_name = experiment_name;
}

void BaseDatapath::getCommonConfigParameters(int& unrolling_factor,
                                             bool& pipelining,
                                             int& partition_factor) {
  // First, collect pipelining, unrolling, and partitioning parameters. We'll
  // assume that all parameters are uniform for all loops, and we'll insert a
  // path to the actual config file if we need to look up the actual
  // configurations.
  std::unordered_map<int, int> unrolling_config;
  BaseDatapath::readUnrollingConfig(unrolling_config);
  unrolling_factor =
      unrolling_config.empty() ? 1 : unrolling_config.begin()->second;
  pipelining = readPipeliningConfig();
  std::unordered_map<std::string, partitionEntry> partition_config;
  BaseDatapath::readPartitionConfig(partition_config);
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
