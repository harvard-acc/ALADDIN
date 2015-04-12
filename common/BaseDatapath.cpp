#include <sstream>
#include <boost/tokenizer.hpp>

#include "opcode_func.h"
#include "BaseDatapath.h"

BaseDatapath::BaseDatapath(std::string bench, std::string trace_file,
                           std::string config_file) {
  benchName = (char *)bench.c_str();
  this->trace_file = trace_file;
  this->config_file = config_file;
  DDDG *dddg;
  dddg = new DDDG(this, trace_file);
  /*Build Initial DDDG*/
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
  numTotalNodes = microop.size();

  BGL_FORALL_VERTICES(v, graph_, Graph)
  nameToVertex[get(boost::vertex_index, graph_, v)] = v;
  vertexToName = get(boost::vertex_index, graph_);

  std::vector<std::string> dynamic_methodid(numTotalNodes, "");
  initDynamicMethodID(dynamic_methodid);

  for (auto dynamic_func_it = dynamic_methodid.begin(),
            E = dynamic_methodid.end();
       dynamic_func_it != E; dynamic_func_it++) {
    char func_id[256];
    int count;
    sscanf((*dynamic_func_it).c_str(), "%[^-]-%d\n", func_id, &count);
    if (functionNames.find(func_id) == functionNames.end())
      functionNames.insert(func_id);
  }
  parse_config(bench, config_file);

  num_cycles = 0;
}

BaseDatapath::~BaseDatapath() {}

void BaseDatapath::addDddgEdge(unsigned int from, unsigned int to,
                               uint8_t parid) {
  if (from != to)
    add_edge(from, to, EdgeProperty(parid), graph_);
}
// optimizationFunctions
void BaseDatapath::setGlobalGraph() {

  std::cerr << "=============================================" << std::endl;
  std::cerr << "      Optimizing...            " << benchName << std::endl;
  std::cerr << "=============================================" << std::endl;
  finalIsolated.assign(numTotalNodes, 1);
}

void BaseDatapath::memoryAmbiguation() {
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "      Memory Ambiguation       " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  std::unordered_multimap<std::string, std::string> pair_per_load;
  std::unordered_set<std::string> paired_store;
  std::unordered_map<std::string, bool> store_load_pair;

  std::vector<std::string> instid(numTotalNodes, "");
  std::vector<std::string> dynamic_methodid(numTotalNodes, "");

  initInstID(instid);
  initDynamicMethodID(dynamic_methodid);
  std::vector<Vertex> topo_nodes;
  boost::topological_sort(graph_, std::back_inserter(topo_nodes));
  // nodes with no incoming edges to first
  for (auto vi = topo_nodes.rbegin(); vi != topo_nodes.rend(); ++vi) {
    unsigned node_id = vertexToName[*vi];

    int node_microop = microop.at(node_id);
    if (is_load_op(node_microop)) {
      // iterate its children to find gep, if the current load generates the
      // address for its grandchildren, the dependences cannot be eliminated in
      // later
      // optimization.
      out_edge_iter out_edge_it, out_edge_end;
      for (tie(out_edge_it, out_edge_end) = out_edges(*vi, graph_);
           out_edge_it != out_edge_end; ++out_edge_it) {
        Vertex child_node = target(*out_edge_it, graph_);
        int child_id = vertexToName[child_node];
        int child_microop = microop.at(child_id);
        if (child_microop != LLVM_IR_GetElementPtr)
          continue;
        out_edge_iter gep_out_edge_it, gep_out_edge_end;
        for (tie(gep_out_edge_it, gep_out_edge_end) =
                 out_edges(child_node, graph_);
             gep_out_edge_it != gep_out_edge_end; ++gep_out_edge_it) {
          Vertex grandchild_node = target(*gep_out_edge_it, graph_);
          int grandchild_id = vertexToName[grandchild_node];
          int grandchild_microop = microop.at(grandchild_id);
          if (!is_memory_op(grandchild_microop))
            continue;
          dynamicMemoryOps.insert(grandchild_id);
        }
      }
    } else if (is_store_op(node_microop)) {
      // iterate its children to find a load op
      out_edge_iter out_edge_it, out_edge_end;
      for (tie(out_edge_it, out_edge_end) = out_edges(*vi, graph_);
           out_edge_it != out_edge_end; ++out_edge_it) {
        int child_id = vertexToName[target(*out_edge_it, graph_)];
        int child_microop = microop.at(child_id);
        if (!is_load_op(child_microop))
          continue;
        std::string node_dynamic_methodid = dynamic_methodid.at(node_id);
        std::string load_dynamic_methodid = dynamic_methodid.at(child_id);
        if (node_dynamic_methodid.compare(load_dynamic_methodid) != 0)
          continue;

        std::string store_unique_id(node_dynamic_methodid + "-" +
                                    instid.at(node_id));
        std::string load_unique_id(load_dynamic_methodid + "-" +
                                   instid.at(child_id));

        if (store_load_pair.find(store_unique_id + "-" + load_unique_id) !=
            store_load_pair.end())
          continue;
        // add to the pair
        store_load_pair[store_unique_id + "-" + load_unique_id] = 1;
        paired_store.insert(store_unique_id);
        auto load_range = pair_per_load.equal_range(load_unique_id);
        bool found_store = 0;
        for (auto store_it = load_range.first; store_it != load_range.second;
             store_it++) {
          if (store_unique_id.compare(store_it->second) == 0) {
            found_store = 1;
            break;
          }
        }
        if (!found_store)
          pair_per_load.insert(make_pair(load_unique_id, store_unique_id));
      }
    }
  }
  if (store_load_pair.size() == 0)
    return;

  std::vector<newEdge> to_add_edges;
  std::unordered_map<std::string, unsigned> last_store;

  for (unsigned node_id = 0; node_id < numTotalNodes; node_id++) {
    int node_microop = microop.at(node_id);
    if (!is_memory_op(node_microop))
      continue;
    std::string unique_id(dynamic_methodid.at(node_id) + "-" +
                          instid.at(node_id));
    if (is_store_op(node_microop)) {
      auto store_it = paired_store.find(unique_id);
      if (store_it == paired_store.end())
        continue;
      last_store[unique_id] = node_id;
    } else {
      assert(is_load_op(node_microop));
      auto load_range = pair_per_load.equal_range(unique_id);
      if (std::distance(load_range.first, load_range.second) == 1)
        continue;
      for (auto load_store_it = load_range.first;
           load_store_it != load_range.second; ++load_store_it) {
        assert(paired_store.find(load_store_it->second) != paired_store.end());
        auto prev_store_it = last_store.find(load_store_it->second);
        if (prev_store_it == last_store.end())
          continue;
        unsigned prev_store_id = prev_store_it->second;
        if (!doesEdgeExist(prev_store_id, node_id))
        {
          to_add_edges.push_back({prev_store_id, node_id, -1});
          dynamicMemoryOps.insert(prev_store_id);
          dynamicMemoryOps.insert(node_id);
        }
      }
    }
  }
  updateGraphWithNewEdges(to_add_edges);
}
/*
 * Read: graph_, microop
 * Modify: graph_
 */
void BaseDatapath::removePhiNodes() {
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "  Remove PHI and Convert Nodes " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);
  std::set<Edge> to_remove_edges;
  std::vector<newEdge> to_add_edges;
  std::unordered_set<unsigned> checked_phi_nodes;

  for (int node_id = nameToVertex.size(); node_id >= 0; node_id--) {
    if (nameToVertex.find(node_id) == nameToVertex.end())
      continue;
    Vertex node_vertex = nameToVertex[node_id];
    int node_microop = microop.at(node_id);
    if (node_microop != LLVM_IR_PHI && !is_convert_op(node_microop))
      continue;
    if (checked_phi_nodes.find(node_id) != checked_phi_nodes.end())
      continue;
    // find its children
    std::vector<pair<unsigned, int> > phi_child;

    out_edge_iter out_edge_it, out_edge_end;
    for (tie(out_edge_it, out_edge_end) = out_edges(node_vertex, graph_);
         out_edge_it != out_edge_end; ++out_edge_it) {
      checked_phi_nodes.insert(node_id);
      to_remove_edges.insert(*out_edge_it);
      phi_child.push_back(make_pair(vertexToName[target(*out_edge_it, graph_)],
                                    edge_to_parid[*out_edge_it]));
    }
    if (phi_child.size() == 0 || boost::in_degree(node_vertex, graph_) == 0)
      continue;
    if (node_microop == LLVM_IR_PHI) {
      // find its parent: phi node can have multiple children, but can only have
      // one parent.
      assert(boost::in_degree(node_vertex, graph_) == 1);
      in_edge_iter in_edge_it = in_edges(node_vertex, graph_).first;
      unsigned parent_id = vertexToName[source(*in_edge_it, graph_)];
      while (microop.at(parent_id) == LLVM_IR_PHI) {
        checked_phi_nodes.insert(parent_id);
        to_remove_edges.insert(*in_edge_it);
        Vertex parent_vertex = nameToVertex[parent_id];
        in_edge_it = in_edges(parent_vertex, graph_).first;
        parent_id = vertexToName[source(*in_edge_it, graph_)];
      }
      to_remove_edges.insert(*in_edge_it);
      for (auto child_it = phi_child.begin(), chil_E = phi_child.end();
           child_it != chil_E; ++child_it) {
        unsigned child_id = child_it->first;
        to_add_edges.push_back({ parent_id, child_id, child_it->second });
      }
    } else {
      // convert nodes
      in_edge_iter in_edge_it, in_edge_end;
      for (tie(in_edge_it, in_edge_end) = in_edges(node_vertex, graph_);
           in_edge_it != in_edge_end; ++in_edge_it) {
        unsigned parent_id = vertexToName[source(*in_edge_it, graph_)];
        to_remove_edges.insert(*in_edge_it);
        for (auto child_it = phi_child.begin(), chil_E = phi_child.end();
             child_it != chil_E; ++child_it) {
          unsigned child_id = child_it->first;
          to_add_edges.push_back({ parent_id, child_id, child_it->second });
        }
      }
    }
    std::vector<pair<unsigned, int> >().swap(phi_child);
  }

  updateGraphWithNewEdges(to_add_edges);
  updateGraphWithIsolatedEdges(to_remove_edges);
  cleanLeafNodes();
}
/*
 * Read: lineNum.gz, flattenConfig, microop
 * Modify: graph_
 */
void BaseDatapath::loopFlatten() {
  std::unordered_set<int> flatten_config;
  if (!readFlattenConfig(flatten_config))
    return;
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "         Loop Flatten          " << std::endl;
  std::cerr << "-------------------------------" << std::endl;
  std::vector<int> lineNum(numTotalNodes, -1);
  initLineNum(lineNum);

  std::vector<unsigned> to_remove_nodes;

  for (unsigned node_id = 0; node_id < numTotalNodes; node_id++) {
    int node_linenum = lineNum.at(node_id);
    auto it = flatten_config.find(node_linenum);
    if (it == flatten_config.end())
      continue;
    if (is_compute_op(microop.at(node_id)))
      microop.at(node_id) = LLVM_IR_Move;
    else if (is_branch_op(microop.at(node_id)))
      to_remove_nodes.push_back(node_id);
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
    int node_microop = microop.at(node_id);
    if (num_of_children.at(node_id) == boost::out_degree(node_vertex, graph_) &&
        node_microop != LLVM_IR_SilentStore && node_microop != LLVM_IR_Store &&
        node_microop != LLVM_IR_Ret && !is_branch_op(node_microop)) {
      to_remove_nodes.push_back(node_id);
      // iterate its parents
      in_edge_iter in_edge_it, in_edge_end;
      for (tie(in_edge_it, in_edge_end) = in_edges(node_vertex, graph_);
           in_edge_it != in_edge_end; ++in_edge_it) {
        int parent_id = vertexToName[source(*in_edge_it, graph_)];
        num_of_children.at(parent_id)++;
      }
    } else if (is_branch_op(node_microop)) {
      // iterate its parents
      in_edge_iter in_edge_it, in_edge_end;
      for (tie(in_edge_it, in_edge_end) = in_edges(node_vertex, graph_);
           in_edge_it != in_edge_end; ++in_edge_it) {
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
 * Read: graph_, instid, microop
 * Modify: microop
 */
void BaseDatapath::removeInductionDependence() {
  // set graph
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "  Remove Induction Dependence  " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  std::vector<std::string> instid(numTotalNodes, "");
  initInstID(instid);

  std::vector<Vertex> topo_nodes;
  boost::topological_sort(graph_, std::back_inserter(topo_nodes));
  // nodes with no incoming edges to first
  for (auto vi = topo_nodes.rbegin(); vi != topo_nodes.rend(); ++vi) {
    unsigned node_id = vertexToName[*vi];
    std::string node_instid = instid.at(node_id);

    if (node_instid.find("indvars") == std::string::npos)
      continue;
    if (microop.at(node_id) == LLVM_IR_Add)
      microop.at(node_id) = LLVM_IR_IndexAdd;
  }
}

// called in the end of the whole flow
void BaseDatapath::dumpStats() {
  clearGraph();
  dumpGraph(benchName);
  writeMicroop(microop);
  writeFinalLevel();
  writeGlobalIsolated();
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
  // block,
  // where all the instructions in the following basic block depend on the prev
  // branch instruction
  // To support loop pipelining, which allows the next iteration
  // starting without waiting until the prev iteration finish, we move the
  // control dependences
  // between last branch node in the prev basic block and instructions in the
  // next basic block
  // to first non isolated instruction in the prev basic block and instructions
  // in the next basic block...
  std::map<unsigned, unsigned> first_non_isolated_node;
  auto bound_it = loopBound.begin();
  unsigned node_id = *bound_it;
  // skip first region
  bound_it++;
  while ((unsigned)node_id < numTotalNodes) {
    assert(is_branch_op(microop.at(*bound_it)));
    while (node_id < *bound_it && (unsigned)node_id < numTotalNodes) {
      if (nameToVertex.find(node_id) == nameToVertex.end() ||
          boost::degree(nameToVertex[node_id], graph_) == 0 ||
          is_branch_op(microop.at(node_id))) {
        node_id++;
        continue;
      } else {
        first_non_isolated_node[*bound_it] = node_id;
        node_id = *bound_it;
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
  int prev_branch = -1;
  int prev_first = -1;
  for (auto first_it = first_non_isolated_node.begin(),
            E = first_non_isolated_node.end();
       first_it != E; ++first_it) {
    unsigned br_node = first_it->first;
    unsigned first_id = first_it->second;
    if (is_call_op(microop.at(br_node))) {
      prev_branch = -1;
      continue;
    }
    if (prev_branch != -1) {
      // adding dependence between prev_first and first_id
      if (!doesEdgeExist(prev_first, first_id))
        to_add_edges.push_back({(unsigned)prev_first, first_id, CONTROL_EDGE });
      // adding dependence between first_id and prev_branch's children
      out_edge_iter out_edge_it, out_edge_end;
      for (tie(out_edge_it, out_edge_end) =
               out_edges(nameToVertex[prev_branch], graph_);
           out_edge_it != out_edge_end; ++out_edge_it) {
        Vertex child_vertex = target(*out_edge_it, graph_);
        unsigned child_id = vertexToName[child_vertex];
        if (child_id <= first_id || edge_to_parid[*out_edge_it] != CONTROL_EDGE)
          continue;
        if (!doesEdgeExist(first_id, child_id))
          to_add_edges.push_back({ first_id, child_id, 1 });
      }
    }
    // update first_id's parents, dependence become strict control dependence
    in_edge_iter in_edge_it, in_edge_end;
    for (tie(in_edge_it, in_edge_end) =
             in_edges(nameToVertex[first_id], graph_);
         in_edge_it != in_edge_end; ++in_edge_it) {
      Vertex parent_vertex = source(*in_edge_it, graph_);
      unsigned parent_id = vertexToName[parent_vertex];
      if (is_branch_op(microop.at(parent_id)))
        continue;
      to_remove_edges.insert(*in_edge_it);
      to_add_edges.push_back({ parent_id, first_id, CONTROL_EDGE });
    }
    // remove control dependence between br node to its children
    out_edge_iter out_edge_it, out_edge_end;
    for (tie(out_edge_it, out_edge_end) =
             out_edges(nameToVertex[br_node], graph_);
         out_edge_it != out_edge_end; ++out_edge_it) {
      if (is_call_op(microop.at(vertexToName[target(*out_edge_it, graph_)])))
        continue;
      if (edge_to_parid[*out_edge_it] != CONTROL_EDGE)
        continue;
      to_remove_edges.insert(*out_edge_it);
    }
    prev_branch = br_node;
    prev_first = first_id;
  }

  updateGraphWithNewEdges(to_add_edges);
  updateGraphWithIsolatedEdges(to_remove_edges);
  cleanLeafNodes();
}
/*
 * Read: graph_, lineNum.gz, unrollingConfig, microop
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
  std::vector<unsigned> nodes_between;
  std::vector<newEdge> to_add_edges;
  std::vector<int> lineNum(numTotalNodes, -1);
  initLineNum(lineNum);

  bool first = false;
  int iter_counts = 0;
  int prev_branch = -1;

  for (unsigned node_id = 0; node_id < numTotalNodes; node_id++) {
    if (nameToVertex.find(node_id) == nameToVertex.end())
      continue;
    Vertex node_vertex = nameToVertex[node_id];
    if (boost::degree(node_vertex, graph_) == 0 &&
        !is_call_op(microop.at(node_id)))
      continue;
    if (!first) {
      first = true;
      loopBound.push_back(node_id);
      prev_branch = node_id;
    }
    assert(prev_branch != -1);
    if (prev_branch != node_id && !(is_dma_op(microop.at(prev_branch)) &&
                                    is_dma_op(microop.at(node_id)))) {
      to_add_edges.push_back({(unsigned)prev_branch, node_id, CONTROL_EDGE });
    }
    if (!is_branch_op(microop.at(node_id)))
      nodes_between.push_back(node_id);
    else {
      // for the case that the first non-isolated node is also a call node;
      if (is_call_op(microop.at(node_id)) && *loopBound.rbegin() != node_id) {
        loopBound.push_back(node_id);
        prev_branch = node_id;
      }

      int node_linenum = lineNum.at(node_id);
      auto unroll_it = unrolling_config.find(node_linenum);
      // not unrolling branch
      if (unroll_it == unrolling_config.end()) {
        if (!is_call_op(microop.at(node_id))) {
          nodes_between.push_back(node_id);
          continue;
        }
        // Enforce dependences between branch nodes, including call nodes
        // Except for the case that both two branches are DMA operations.
        // (Two DMA operations can go in parallel.)
        if (!doesEdgeExist(prev_branch, node_id) &&
            !(is_dma_op(microop.at(prev_branch)) &&
              is_dma_op(microop.at(node_id))))
          to_add_edges.push_back(
              {(unsigned)prev_branch, node_id, CONTROL_EDGE });
        for (auto prev_node_it = nodes_between.begin(), E = nodes_between.end();
             prev_node_it != E; prev_node_it++) {
          if (!doesEdgeExist(*prev_node_it, node_id) &&
              !(is_dma_op(microop.at(*prev_node_it)) &&
                is_dma_op(microop.at(node_id)))) {
            to_add_edges.push_back({ *prev_node_it, node_id, CONTROL_EDGE });
          }
        }
        nodes_between.clear();
        nodes_between.push_back(node_id);
        prev_branch = node_id;
      } else {
        int factor = unroll_it->second;
        int node_microop = microop.at(node_id);
        char unique_inst_id[256];
        sprintf(unique_inst_id, "%d-%d", node_microop, node_linenum);
        auto it = inst_dynamic_counts.find(unique_inst_id);
        if (it == inst_dynamic_counts.end()) {
          inst_dynamic_counts[unique_inst_id] = 1;
          it = inst_dynamic_counts.find(unique_inst_id);
        } else
          it->second++;
        if (it->second % factor == 0) {
          loopBound.push_back(node_id);
          iter_counts++;
          for (auto prev_node_it = nodes_between.begin(),
                    E = nodes_between.end();
               prev_node_it != E; prev_node_it++) {
            if (!doesEdgeExist(*prev_node_it, node_id)) {
              to_add_edges.push_back({ *prev_node_it, node_id, CONTROL_EDGE });
            }
          }
          nodes_between.clear();
          nodes_between.push_back(node_id);
          prev_branch = node_id;
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
 * Read: loop_bound, flattenConfig, graph, actualAddress, microop
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

  std::unordered_map<unsigned, MemAccess> address;
  initAddress(address);

  vertex_iter vi, vi_end;

  std::set<Edge> to_remove_edges;
  std::vector<newEdge> to_add_edges;

  int shared_loads = 0;
  auto bound_it = loopBound.begin();

  unsigned node_id = 0;
  while ((unsigned)node_id < numTotalNodes) {
    std::unordered_map<unsigned, unsigned> address_loaded;
    while (node_id < *bound_it && (unsigned)node_id < numTotalNodes) {
      if (nameToVertex.find(node_id) == nameToVertex.end() ||
          boost::degree(nameToVertex[node_id], graph_) == 0) {
        node_id++;
        continue;
      }
      int node_microop = microop.at(node_id);
      long long int node_address = address[node_id].vaddr;
      auto addr_it = address_loaded.find(node_address);
      if (is_store_op(node_microop) && addr_it != address_loaded.end())
        address_loaded.erase(addr_it);
      else if (is_load_op(node_microop)) {
        if (addr_it == address_loaded.end())
          address_loaded[node_address] = node_id;
        else {
          //check whether the current load is dynamic or not.
          if (dynamicMemoryOps.find(node_id) != dynamicMemoryOps.end()) {
            ++node_id;
            continue;
          }
          shared_loads++;
          microop.at(node_id) = LLVM_IR_Move;
          unsigned prev_load = addr_it->second;
          // iterate through its children
          Vertex load_node = nameToVertex[node_id];
          out_edge_iter out_edge_it, out_edge_end;
          for (tie(out_edge_it, out_edge_end) = out_edges(load_node, graph_);
               out_edge_it != out_edge_end; ++out_edge_it) {
            Edge curr_edge = *out_edge_it;
            Vertex child_vertex = target(curr_edge, graph_);
            unsigned child_id = vertexToName[child_vertex];
            Vertex prev_load_vertex = nameToVertex[prev_load];
            if (!doesEdgeExistVertex(prev_load_vertex, child_vertex))
              to_add_edges.push_back(
                  { prev_load, child_id, edge_to_parid[curr_edge] });
            to_remove_edges.insert(*out_edge_it);
          }
          in_edge_iter in_edge_it, in_edge_end;
          for (tie(in_edge_it, in_edge_end) = in_edges(load_node, graph_);
               in_edge_it != in_edge_end; ++in_edge_it)
            to_remove_edges.insert(*in_edge_it);
        }
      }
      node_id++;
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

  unsigned node_id = 0;
  while (node_id < numTotalNodes) {
    while (node_id < *bound_it && node_id < numTotalNodes) {
      if (nameToVertex.find(node_id) == nameToVertex.end() ||
          boost::degree(nameToVertex[node_id], graph_) == 0) {
        ++node_id;
        continue;
      }
      if (is_store_op(microop.at(node_id)))
      {
        //remove this store
        //dynamic stores, cannot disambiguated in the static time, cannot remove
        if (dynamicMemoryOps.find(node_id) != dynamicMemoryOps.end()) {
          ++node_id;
          continue;
        }
        Vertex node = nameToVertex[node_id];
        out_edge_iter out_edge_it, out_edge_end;

        std::vector<Vertex> store_child;
        for (tie(out_edge_it, out_edge_end) = out_edges(node, graph_);
             out_edge_it != out_edge_end; ++out_edge_it) {
          Vertex child_vertex = target(*out_edge_it, graph_);
          int child_id = vertexToName[child_vertex];
          if (is_load_op(microop.at(child_id)))
          {
            if (dynamicMemoryOps.find(child_id) != dynamicMemoryOps.end()
               || child_id >= (unsigned)*bound_it )
              continue;
            else
              store_child.push_back(child_vertex);
          }
        }

        if (store_child.size() > 0) {
          bool parent_found = false;
          Vertex store_parent;
          in_edge_iter in_edge_it, in_edge_end;
          for (tie(in_edge_it, in_edge_end) = in_edges(node, graph_);
               in_edge_it != in_edge_end; ++in_edge_it) {
            // parent node that generates value
            if (edge_to_parid[*in_edge_it] == 1) {
              parent_found = true;
              store_parent = source(*in_edge_it, graph_);
              break;
            }
          }

          if (parent_found) {
            for (auto load_it = store_child.begin(), E = store_child.end();
                 load_it != E; ++load_it) {
              Vertex load_node = *load_it;
              to_remove_nodes.push_back(vertexToName[load_node]);

              out_edge_iter out_edge_it, out_edge_end;
              for (tie(out_edge_it, out_edge_end) =
                       out_edges(load_node, graph_);
                   out_edge_it != out_edge_end; ++out_edge_it) {
                to_add_edges.push_back(
                    {(unsigned)vertexToName[store_parent],
                     (unsigned)vertexToName[target(*out_edge_it, graph_)],
                     edge_to_parid[*out_edge_it] });
              }
            }
          }
        }
      }
      ++node_id;
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

  std::unordered_map<unsigned, MemAccess> address;
  initAddress(address);

  int node_id = numTotalNodes - 1;
  auto bound_it = loopBound.end();
  bound_it--;
  bound_it--;
  while (node_id >= 0) {
    unordered_map<unsigned, int> address_store_map;

    while (node_id >= *bound_it && node_id >= 0) {
      if (nameToVertex.find(node_id) == nameToVertex.end() ||
          boost::degree(nameToVertex[node_id], graph_) == 0 ||
          !is_store_op(microop.at(node_id))) {
        --node_id;
        continue;
      }
      long long int node_address = address[node_id].vaddr;
      auto addr_it = address_store_map.find(node_address);

      if (addr_it == address_store_map.end())
        address_store_map[node_address] = node_id;
      else {
        //remove this store
        //dynamic stores, cannot disambiguated in the run time, cannot remove
        if (dynamicMemoryOps.find(node_id) == dynamicMemoryOps.end() ) {
          if (boost::out_degree(nameToVertex[node_id], graph_)== 0) {
            microop.at(node_id) = LLVM_IR_SilentStore;
          } else {
            int num_of_real_children = 0;
            out_edge_iter out_edge_it, out_edge_end;
            for (tie(out_edge_it, out_edge_end) =
                     out_edges(nameToVertex[node_id], graph_);
                 out_edge_it != out_edge_end; ++out_edge_it) {
              if (edge_to_parid[*out_edge_it] != CONTROL_EDGE)
                num_of_real_children++;
            }
            if (num_of_real_children == 0) {
              microop.at(node_id) = LLVM_IR_SilentStore;
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
 * Read: loopBound, flattenConfig, graph_, microop
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
  for (int node_id = numTotalNodes - 1; node_id >= 0; node_id--) {
    if (nameToVertex.find(node_id) == nameToVertex.end() ||
        boost::degree(nameToVertex[node_id], graph_) == 0 ||
        updated.at(node_id) || !is_associative(microop.at(node_id)))
      continue;
    updated.at(node_id) = 1;
    int node_region = bound_region.at(node_id);

    std::list<unsigned> nodes;
    std::vector<Edge> tmp_remove_edges;
    std::vector<pair<int, bool> > leaves;
    std::vector<int> associative_chain;
    associative_chain.push_back(node_id);
    int chain_id = 0;
    while (chain_id < associative_chain.size()) {
      int chain_node_id = associative_chain.at(chain_id);
      int chain_node_microop = microop.at(chain_node_id);
      if (is_associative(chain_node_microop)) {
        updated.at(chain_node_id) = 1;
        int num_of_chain_parents = 0;
        in_edge_iter in_edge_it, in_edge_end;
        for (tie(in_edge_it, in_edge_end) =
                 in_edges(nameToVertex[chain_node_id], graph_);
             in_edge_it != in_edge_end; ++in_edge_it) {
          int parent_id = vertexToName[source(*in_edge_it, graph_)];
          if (is_branch_op(microop.at(parent_id)))
            continue;
          num_of_chain_parents++;
        }
        if (num_of_chain_parents == 2) {
          nodes.push_front(chain_node_id);
          for (tie(in_edge_it, in_edge_end) =
                   in_edges(nameToVertex[chain_node_id], graph_);
               in_edge_it != in_edge_end; ++in_edge_it) {
            Vertex parent_node = source(*in_edge_it, graph_);
            int parent_id = vertexToName[parent_node];
            assert(parent_id < chain_node_id);
            int parent_region = bound_region.at(parent_id);
            int parent_microop = microop.at(parent_id);
            if (is_branch_op(parent_microop))
              continue;
            Edge curr_edge = *in_edge_it;
            tmp_remove_edges.push_back(curr_edge);

            if (parent_region == node_region) {
              updated.at(parent_id) = 1;
              if (!is_associative(parent_microop))
                leaves.push_back(make_pair(parent_id, 0));
              else {
                out_edge_iter out_edge_it, out_edge_end;
                int num_of_children = 0;
                for (tie(out_edge_it, out_edge_end) =
                         out_edges(parent_node, graph_);
                     out_edge_it != out_edge_end; ++out_edge_it) {
                  if (edge_to_parid[*out_edge_it] != CONTROL_EDGE)
                    num_of_children++;
                }

                if (num_of_children == 1)
                  associative_chain.push_back(parent_id);
                else
                  leaves.push_back(make_pair(parent_id, 0));
              }
            } else
              leaves.push_back(make_pair(parent_id, 1));
          }
        } else
          leaves.push_back(make_pair(chain_node_id, 0));
      } else
        leaves.push_back(make_pair(chain_node_id, 0));
      chain_id++;
    }
    // build the tree
    if (nodes.size() < 3)
      continue;

    for (auto it = tmp_remove_edges.begin(), E = tmp_remove_edges.end();
         it != E; it++)
      to_remove_edges.insert(*it);

    std::map<unsigned, unsigned> rank_map;
    auto leaf_it = leaves.begin();

    while (leaf_it != leaves.end()) {
      if (leaf_it->second == 0)
        rank_map[leaf_it->first] = 0;
      else
        rank_map[leaf_it->first] = numTotalNodes;
      ++leaf_it;
    }
    // reconstruct the rest of the balanced tree
    auto node_it = nodes.begin();

    while (node_it != nodes.end()) {
      unsigned node1, node2;
      if (rank_map.size() == 2) {
        node1 = rank_map.begin()->first;
        node2 = (++rank_map.begin())->first;
      } else
        findMinRankNodes(node1, node2, rank_map);
      assert((node1 != numTotalNodes) && (node2 != numTotalNodes));
      to_add_edges.push_back({ node1, *node_it, 1 });
      to_add_edges.push_back({ node2, *node_it, 1 });

      // place the new node in the map, remove the two old nodes
      rank_map[*node_it] = max(rank_map[node1], rank_map[node2]) + 1;
      rank_map.erase(node1);
      rank_map.erase(node2);
      ++node_it;
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
void BaseDatapath::findMinRankNodes(unsigned &node1, unsigned &node2,
                                    std::map<unsigned, unsigned> &rank_map) {
  unsigned min_rank = numTotalNodes;
  for (auto it = rank_map.begin(); it != rank_map.end(); ++it) {
    int node_rank = it->second;
    if (node_rank < min_rank) {
      node1 = it->first;
      min_rank = node_rank;
    }
  }
  min_rank = numTotalNodes;
  for (auto it = rank_map.begin(); it != rank_map.end(); ++it) {
    int node_rank = it->second;
    if ((it->first != node1) && (node_rank < min_rank)) {
      node2 = it->first;
      min_rank = node_rank;
    }
  }
}

void BaseDatapath::updateGraphWithNewEdges(std::vector<newEdge> &to_add_edges) {
  for (auto it = to_add_edges.begin(); it != to_add_edges.end(); ++it) {
    if (it->from != it->to && !doesEdgeExist(it->from, it->to))
      get(boost::edge_name, graph_)[add_edge(it->from, it->to, graph_).first] =
          it->parid;
  }
}
void BaseDatapath::updateGraphWithIsolatedNodes(
    std::vector<unsigned> &to_remove_nodes) {
  for (auto it = to_remove_nodes.begin(); it != to_remove_nodes.end(); ++it)
    clear_vertex(nameToVertex[*it], graph_);
}
void
BaseDatapath::updateGraphWithIsolatedEdges(std::set<Edge> &to_remove_edges) {
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

  initPerCycleActivity(comp_partition_names, mem_partition_names, ld_activity,
                       st_activity, mul_activity, add_activity, bit_activity,
                       shifter_activity, max_mul_per_function,
                       max_add_per_function, max_bit_per_function,
                       max_shifter_per_function, num_cycles);

  updatePerCycleActivity(ld_activity, st_activity, mul_activity, add_activity,
                         bit_activity, shifter_activity, max_mul_per_function,
                         max_add_per_function, max_bit_per_function,
                         max_shifter_per_function);

  outputPerCycleActivity(comp_partition_names, mem_partition_names, ld_activity,
                         st_activity, mul_activity, add_activity, bit_activity,
                         shifter_activity, max_mul_per_function,
                         max_add_per_function, max_bit_per_function,
                         max_shifter_per_function);
}

void BaseDatapath::initPerCycleActivity(
    std::vector<std::string> &comp_partition_names,
    std::vector<std::string> &mem_partition_names, activity_map &ld_activity,
    activity_map &st_activity, activity_map &mul_activity,
    activity_map &add_activity, activity_map &bit_activity,
    activity_map &shifter_activity, max_activity_map &max_mul_per_function,
    max_activity_map &max_add_per_function,
    max_activity_map &max_bit_per_function,
    max_activity_map &max_shifter_per_function, int num_cycles) {
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
    activity_map &ld_activity, activity_map &st_activity,
    activity_map &mul_activity, activity_map &add_activity,
    activity_map &bit_activity, activity_map &shifter_activity,
    max_activity_map &max_mul_per_function,
    max_activity_map &max_add_per_function,
    max_activity_map &max_bit_per_function,
    max_activity_map &max_shifter_per_function) {
  /*We use two ways to count the number of functional units in accelerators: one
   * assumes that functional units can be reused in the same region; the other
   * assumes no reuse of functional units. The advantage of reusing is that it
   * elimates the cost of duplicating functional units which can lead to high
   * leakage power and area. However, additional wires and muxes may need to be
   * added for reusing.
   * In the current model, we assume that multipliers can be reused, since the
   * leakage power and area of multipliers are relatively significant, and no
   * reuse for adders. This way of modeling is consistent with our observation
   * of accelerators generated with Vivado.*/
  std::vector<std::string> dynamic_methodid(numTotalNodes, "");
  initDynamicMethodID(dynamic_methodid);

  int num_adds_so_far = 0, num_muls_so_far = 0, num_bits_so_far = 0;
  int num_shifters_so_far = 0;
  auto bound_it = loopBound.begin();
  for (unsigned node_id = 0; node_id < numTotalNodes; ++node_id) {
    char func_id[256];
    int count;

    sscanf(dynamic_methodid.at(node_id).c_str(), "%[^-]-%d\n", func_id, &count);
    if (node_id == *bound_it) {
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
    if (finalIsolated.at(node_id))
      continue;
    int node_level = newLevel.at(node_id);
    int node_microop = microop.at(node_id);

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
      std::string base_addr = nodeToLabel[node_id];
      if (ld_activity.find(base_addr) != ld_activity.end())
        ld_activity[base_addr].at(node_level) += 1;
    } else if (is_store_op(node_microop)) {
      std::string base_addr = nodeToLabel[node_id];
      if (st_activity.find(base_addr) != st_activity.end())
        st_activity[base_addr].at(node_level) += 1;
    }
  }
  for (auto it = functionNames.begin(); it != functionNames.end(); ++it)
    max_mul_per_function[*it] =
        *(std::max_element(mul_activity[*it].begin(), mul_activity[*it].end()));
}

void BaseDatapath::outputPerCycleActivity(
    std::vector<std::string> &comp_partition_names,
    std::vector<std::string> &mem_partition_names, activity_map &ld_activity,
    activity_map &st_activity, activity_map &mul_activity,
    activity_map &add_activity, activity_map &bit_activity,
    activity_map &shifter_activity, max_activity_map &max_mul_per_function,
    max_activity_map &max_add_per_function,
    max_activity_map &max_bit_per_function,
    max_activity_map &max_shifter_per_function) {
  /*Set the constants*/
  float add_int_power, add_switch_power, add_leak_power, add_area;
  float mul_int_power, mul_switch_power, mul_leak_power, mul_area;
  float reg_int_power_per_bit, reg_switch_power_per_bit;
  float reg_leak_power_per_bit, reg_area_per_bit;
  float bit_int_power, bit_switch_power, bit_leak_power, bit_area;
  float shifter_int_power, shifter_switch_power, shifter_leak_power,
      shifter_area;

  getAdderPowerArea(cycleTime, &add_int_power, &add_switch_power,
                    &add_leak_power, &add_area);
  getMultiplierPowerArea(cycleTime, &mul_int_power, &mul_switch_power,
                         &mul_leak_power, &mul_area);
  getRegisterPowerArea(cycleTime, &reg_int_power_per_bit,
                       &reg_switch_power_per_bit, &reg_leak_power_per_bit,
                       &reg_area_per_bit);
  getBitPowerArea(cycleTime, &bit_int_power, &bit_switch_power, &bit_leak_power,
                  &bit_area);
  getShifterPowerArea(cycleTime, &shifter_int_power, &shifter_switch_power,
                      &shifter_leak_power, &shifter_area);

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
            << bit_activity[*it].at(curr_level) << ",";
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
         it != comp_partition_names.end(); ++it) {
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

  getAverageMemPower(num_cycles, &avg_mem_power, &avg_mem_dynamic_power,
                     &mem_leakage_power);

  float avg_fu_dynamic_power = fu_dynamic_energy / (cycleTime * num_cycles);
  float avg_fu_power = avg_fu_dynamic_power + fu_leakage_power;
  float avg_power = avg_fu_power + avg_mem_power;

  float mem_area = getTotalMemArea();
  unsigned mem_size = getTotalMemSize();
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

  writeSummary(std::cerr, summary);
  ofstream summary_file;
  file_name = bn + "_summary";
  summary_file.open(file_name.c_str());
  writeSummary(summary_file, summary);
  summary_file.close();

}

void BaseDatapath::writeSummary(std::ostream &outfile,
                                summary_data_t &summary) {
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
  outfile << "===============================" << std::endl;
  outfile << "        Aladdin Results        " << std::endl;
  outfile << "===============================" << std::endl;
}

void BaseDatapath::writeGlobalIsolated() {
  std::string file_name(benchName);
  file_name += "_isolated.gz";
  write_gzip_bool_file(file_name, finalIsolated.size(), finalIsolated);
}

void BaseDatapath::writeBaseAddress() {
  ostringstream file_name;
  file_name << benchName << "_baseAddr.gz";
  gzFile gzip_file;
  gzip_file = gzopen(file_name.str().c_str(), "w");
  for (auto it = nodeToLabel.begin(), E = nodeToLabel.end(); it != E; ++it)
    gzprintf(gzip_file, "node:%u,part:%s,base:%lld\n", it->first,
             it->second.c_str(), arrayBaseAddress[it->second]);
  gzclose(gzip_file);
}
void BaseDatapath::writeFinalLevel() {
  std::string file_name(benchName);
  file_name += "_level.gz";
  write_gzip_file(file_name, newLevel.size(), newLevel);
}
void BaseDatapath::writeMicroop(std::vector<int> &microop) {
  std::string file_name(benchName);
  file_name += "_microop.gz";
  write_gzip_file(file_name, microop.size(), microop);
}
void
BaseDatapath::initPrevBasicBlock(std::vector<std::string> &prevBasicBlock) {
  std::string file_name(benchName);
  file_name += "_prevBasicBlock.gz";
  read_gzip_string_file(file_name, prevBasicBlock.size(), prevBasicBlock);
}
void BaseDatapath::initDynamicMethodID(std::vector<std::string> &methodid) {
  std::string file_name(benchName);
  file_name += "_dynamic_funcid.gz";
  read_gzip_string_file(file_name, methodid.size(), methodid);
}
void BaseDatapath::initMethodID(std::vector<int> &methodid) {
  std::string file_name(benchName);
  file_name += "_methodid.gz";
  read_gzip_file(file_name, methodid.size(), methodid);
}
void BaseDatapath::initInstID(std::vector<std::string> &instid) {
  std::string file_name(benchName);
  file_name += "_instid.gz";
  read_gzip_string_file(file_name, instid.size(), instid);
}

void BaseDatapath::initAddress(std::unordered_map<unsigned, MemAccess> &address)
{
  std::string file_name(benchName);
  file_name += "_memaddr.gz";
  gzFile gzip_file;
  gzip_file = gzopen(file_name.c_str(), "r");
  while (!gzeof(gzip_file)) {
    char buffer[256];
    if (gzgets(gzip_file, buffer, 256) == NULL)
      break;
    unsigned node_id, size;
    long long int addr, value;
    int num_filled = sscanf(buffer, "%d,%lld,%d,%lld\n",
                            &node_id, &addr, &size, &value);
    MemAccess access;
    access.vaddr = addr;
    access.size = size;
    access.isLoad = (num_filled == 3);
    if (!access.isLoad)
      access.value = value;
    address[node_id] = access;
  }
  gzclose(gzip_file);
}

void BaseDatapath::initLineNum(std::vector<int> &line_num) {
  ostringstream file_name;
  file_name << benchName << "_linenum.gz";
  read_gzip_file(file_name.str(), line_num.size(), line_num);
}
void BaseDatapath::initGetElementPtr(std::unordered_map<
    unsigned, pair<std::string, long long int> > &get_element_ptr) {
  ostringstream file_name;
  file_name << benchName << "_getElementPtr.gz";
  gzFile gzip_file;
  gzip_file = gzopen(file_name.str().c_str(), "r");
  while (!gzeof(gzip_file)) {
    char buffer[256];
    if (gzgets(gzip_file, buffer, 256) == NULL)
      break;
    unsigned node_id;
    long long int address;
    char label[256];
    sscanf(buffer, "%d,%[^,],%lld\n", &node_id, label, &address);
    get_element_ptr[node_id] = make_pair(label, address);
  }
  gzclose(gzip_file);
}

// stepFunctions
// multiple function, each function is a separate graph
void BaseDatapath::setGraphForStepping() {
  std::cerr << "=============================================" << std::endl;
  std::cerr << "      Scheduling...            " << benchName << std::endl;
  std::cerr << "=============================================" << std::endl;

  newLevel.assign(numTotalNodes, 0);

  edgeToParid = get(boost::edge_name, graph_);

  numTotalEdges = boost::num_edges(graph_);
  numParents.assign(numTotalNodes, 0);
  latestParents.assign(numTotalNodes, 0);
  executedNodes = 0;
  totalConnectedNodes = 0;
  for (unsigned node_id = 0; node_id < nameToVertex.size(); node_id++) {
    Vertex node = nameToVertex[node_id];
    if (boost::degree(node, graph_) != 0 || is_dma_op(microop.at(node_id))) {
      finalIsolated.at(node_id) = 0;
      numParents.at(node_id) = boost::in_degree(node, graph_);
      totalConnectedNodes++;
    }
  }
  executingQueue.clear();
  readyToExecuteQueue.clear();
  initExecutingQueue();
}

void BaseDatapath::dumpGraph(std::string graph_name) {
  std::ofstream out(graph_name + "_graph.dot");
  write_graphviz(out, graph_);
}
int BaseDatapath::clearGraph() {
  std::vector<Vertex> topo_nodes;
  boost::topological_sort(graph_, std::back_inserter(topo_nodes));
  // bottom nodes first
  std::vector<int> earliest_child(numTotalNodes, num_cycles);
  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi) {
    unsigned node_id = vertexToName[*vi];
    if (finalIsolated.at(node_id))
      continue;
    unsigned node_microop = microop.at(node_id);
    if (!is_memory_op(node_microop) && !is_branch_op(node_microop))
      if ((earliest_child.at(node_id) - 1) > newLevel.at(node_id))
        newLevel.at(node_id) = earliest_child.at(node_id) - 1;

    in_edge_iter in_i, in_end;
    for (tie(in_i, in_end) = in_edges(*vi, graph_); in_i != in_end; ++in_i) {
      int parent_id = vertexToName[source(*in_i, graph_)];
      if (earliest_child.at(parent_id) > newLevel.at(node_id))
        earliest_child.at(parent_id) = newLevel.at(node_id);
    }
  }
  updateRegStats();
  return num_cycles;
}
void BaseDatapath::updateRegStats() {
  regStats.assign(num_cycles, { 0, 0, 0 });
  for (unsigned node_id = 0; node_id < nameToVertex.size(); node_id++) {
    if (finalIsolated.at(node_id))
      continue;
    if (is_control_op(microop.at(node_id)) || is_index_op(microop.at(node_id)))
      continue;
    int node_level = newLevel.at(node_id);
    int max_children_level = node_level;

    Vertex node = nameToVertex[node_id];
    out_edge_iter out_edge_it, out_edge_end;
    std::set<int> children_levels;
    for (tie(out_edge_it, out_edge_end) = out_edges(node, graph_);
         out_edge_it != out_edge_end; ++out_edge_it) {
      int child_id = vertexToName[target(*out_edge_it, graph_)];
      int child_microop = microop.at(child_id);
      if (is_control_op(child_microop))
        continue;

      if (is_load_op(child_microop))
        continue;

      int child_level = newLevel.at(child_id);
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
    if (is_store_op(microop.at(*it)))
      executingQueue.push_front(*it);
    else
      executingQueue.push_back(*it);
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
    std::list<unsigned>::iterator &executingQueuePos, int &advance_to) {
  unsigned node_id = *executingQueuePos;
  executedNodes++;
  newLevel.at(node_id) = num_cycles;
  executingQueue.erase(executingQueuePos);
  updateChildren(node_id);
  executingQueuePos = executingQueue.begin();
  std::advance(executingQueuePos, advance_to);
}

void BaseDatapath::updateChildren(unsigned node_id) {
  if (nameToVertex.find(node_id) == nameToVertex.end())
    return;
  Vertex node = nameToVertex[node_id];
  out_edge_iter out_edge_it, out_edge_end;
  for (tie(out_edge_it, out_edge_end) = out_edges(node, graph_);
       out_edge_it != out_edge_end; ++out_edge_it) {
    unsigned child_id = vertexToName[target(*out_edge_it, graph_)];
    int edge_parid = edgeToParid[*out_edge_it];
    if (numParents[child_id] > 0) {
      numParents[child_id]--;
      if (numParents[child_id] == 0) {
        unsigned child_microop = microop.at(child_id);
        if ((node_latency(child_microop) == 0 ||
             node_latency(microop.at(node_id)) == 0) &&
            edge_parid != CONTROL_EDGE)
          executingQueue.push_back(child_id);
        else {
          if (is_store_op(child_microop))
            readyToExecuteQueue.push_front(child_id);
          else
            readyToExecuteQueue.push_back(child_id);
        }

        numParents[child_id] = -1;
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

  std::unordered_map<unsigned, pair<std::string, long long int> > getElementPtr;
  initGetElementPtr(getElementPtr);
  EdgeNameMap edge_to_parid = get(boost::edge_name, graph_);

  vertex_iter vi, vi_end;
  for (tie(vi, vi_end) = vertices(graph_); vi != vi_end; ++vi) {
    if (boost::degree(*vi, graph_) == 0)
      continue;
    Vertex curr_node = *vi;
    unsigned node_id = vertexToName[curr_node];
    int node_microop = microop.at(node_id);
    if (!is_memory_op(node_microop))
      continue;
    bool modified = 0;
    // iterate its parents, until it finds the root parent
    while (true) {
      bool found_parent = 0;
      in_edge_iter in_edge_it, in_edge_end;

      for (tie(in_edge_it, in_edge_end) = in_edges(curr_node, graph_);
           in_edge_it != in_edge_end; ++in_edge_it) {
        int edge_parid = edge_to_parid[*in_edge_it];
        if ((node_microop == LLVM_IR_Load && edge_parid != 1) ||
            (node_microop == LLVM_IR_GetElementPtr && edge_parid != 1) ||
            (node_microop == LLVM_IR_Store && edge_parid != 2))
          continue;

        unsigned parent_id = vertexToName[source(*in_edge_it, graph_)];
        int parent_microop = microop.at(parent_id);
        if (parent_microop == LLVM_IR_GetElementPtr ||
            parent_microop == LLVM_IR_Load) {
          // remove address calculation directly
          std::string label = getElementPtr[parent_id].first;
          long long int addr = getElementPtr[parent_id].second;
          nodeToLabel[node_id] = label;
          arrayBaseAddress[label] = addr;
          curr_node = source(*in_edge_it, graph_);
          node_microop = parent_microop;
          found_parent = 1;
          modified = 1;
          break;
        } else if (parent_microop == LLVM_IR_Alloca) {
          std::string label = getElementPtr[parent_id].first;
          long long int addr = getElementPtr[parent_id].second;
          nodeToLabel[node_id] = label;
          arrayBaseAddress[label] = addr;
          modified = 1;
          break;
        }
      }
      if (!found_parent)
        break;
    }
    if (!modified) {
      std::string label = getElementPtr[node_id].first;
      long long int addr = getElementPtr[node_id].second;
      nodeToLabel[node_id] = label;
      arrayBaseAddress[label] = addr;
    }
  }
  writeBaseAddress();
}

void BaseDatapath::initExecutingQueue() {
  for (unsigned i = 0; i < numTotalNodes; i++) {
    if (numParents[i] == 0 && finalIsolated[i] != 1)
      executingQueue.push_back(i);
  }
}
int BaseDatapath::shortestDistanceBetweenNodes(unsigned int from,
                                               unsigned int to) {
  std::list<pair<unsigned int, unsigned int> > queue;
  queue.push_back({ from, 0 });
  while (queue.size() != 0) {
    unsigned int curr_node = queue.front().first;
    unsigned int curr_dist = queue.front().second;
    out_edge_iter out_edge_it, out_edge_end;
    for (tie(out_edge_it, out_edge_end) =
             out_edges(nameToVertex[curr_node], graph_);
         out_edge_it != out_edge_end; ++out_edge_it) {
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
    std::unordered_map<int, int> &unrolling_config) {
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

bool BaseDatapath::readFlattenConfig(std::unordered_set<int> &flatten_config) {
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
    std::unordered_map<std::string, unsigned> &config) {
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
    std::unordered_map<std::string, partitionEntry> &partition_config) {
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
    sscanf(wholeline.c_str(), "%[^,],%[^,],%d,%d,%d\n", type, base_addr, &size,
           &wordsize, &p_factor);
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
                                  std::vector<int> &tokenized_list) {
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

void BaseDatapath::getCommonConfigParameters(int &unrolling_factor,
                                             bool &pipelining,
                                             int &partition_factor) {
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
int BaseDatapath::getExperimentId(sql::Connection *con) {
  sql::ResultSet *res;
  sql::Statement *stmt = con->createStatement();
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
    query.str(""); // Clear stringstream.
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

int BaseDatapath::getLastInsertId(sql::Connection *con) {
  sql::Statement *stmt = con->createStatement();
  sql::ResultSet *res = stmt->executeQuery("select last_insert_id()");
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

void BaseDatapath::writeSummaryToDatabase(summary_data_t &summary) {
  sql::Driver *driver;
  sql::Connection *con;
  sql::Statement *stmt;
  driver = sql::mysql::get_mysql_driver_instance();
  con = driver->connect(DB_URL, DB_USER, DB_PASS);
  con->setSchema("aladdin");
  con->setAutoCommit(0); // Begin transaction.
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
  con->commit(); // End transaction.
  delete stmt;
  delete con;
}
#endif
