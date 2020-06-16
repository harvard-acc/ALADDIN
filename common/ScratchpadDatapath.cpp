/* Implementation of an accelerator datapath that uses a private scratchpad for
 * local memory.
 */

#include <string>

#include "DatabaseDeps.h"
#include "ScratchpadDatapath.h"

#include "graph_opts/all_graph_opts.h"

ScratchpadDatapath::ScratchpadDatapath(std::string bench,
                                       std::string trace_file,
                                       std::string config_file)
    : BaseDatapath(bench, trace_file, config_file),
      cycle_time(user_params.cycle_time) {
  std::cout << "-------------------------------" << std::endl;
  std::cout << "      Setting ScratchPad       " << std::endl;
  std::cout << "-------------------------------" << std::endl;
  scratchpad = new Scratchpad(user_params.scratchpad_ports,
                              user_params.cycle_time,
                              user_params.ready_mode);
  scratchpadCanService = true;
  mem_reg_conversion_executed = false;
  scratchpad_partition_executed = false;
}

ScratchpadDatapath::~ScratchpadDatapath() { delete scratchpad; }

void ScratchpadDatapath::clearDatapath() {
  BaseDatapath::clearDatapath();
}

void ScratchpadDatapath::globalOptimizationPass() {
  std::cout << "=============================================" << std::endl;
  std::cout << "      Optimizing...            " << benchName << std::endl;
  std::cout << "=============================================" << std::endl;
  // Node removals must come first.
  removePhiNodes();
  /* memoryAmbiguation() should execute after removeInductionDependence()
   * because it needs induction_nodes. */
  removeInductionDependence();
  // Base address must be initialized next.
  initBaseAddress();
  completePartition();
  scratchpadPartition();
  loopFlatten();
  loopUnrolling();
  removeSharedLoads();
  storeBuffer();
  removeRepeatedStores();
  memoryAmbiguation();
  treeHeightReduction();
  fuseRegLoadStores();
  fuseConsecutiveBranches();
  // Must do loop pipelining last; after all the data/control dependences are
  // fixed
  perLoopPipelining();
  loopPipelining();
}

/* First, compute all base addresses, then check each node to make sure that
 * each entry is valid.
 */
void ScratchpadDatapath::initBaseAddress() {
  BaseDatapath::initBaseAddress();
  BaseDatapath::initDmaBaseAddress();

  vertex_iter vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(program.graph); vi != vi_end; ++vi) {
    if (boost::degree(*vi, program.graph) == 0)
      continue;
    Vertex curr_vertex = *vi;
    ExecNode* node = program.nodeAtVertex(curr_vertex);
    if (!node->is_memory_op())
      continue;
    const std::string& part_name = node->get_array_label();
    if (user_params.partition.find(part_name) == user_params.partition.end()) {
      std::cerr << "Unknown partition : " << part_name
                << " at node: " << node->get_node_id() << std::endl;
      exit(-1);
    }
  }
}

/*
 * Modify scratchpad
 */
void ScratchpadDatapath::completePartition() {
  if (mem_reg_conversion_executed)
    return;

  mem_reg_conversion_executed = true;
  if (!user_params.partition.size())
    return;

  std::cout << "-------------------------------" << std::endl;
  std::cout << "        Mem to Reg Conv        " << std::endl;
  std::cout << "-------------------------------" << std::endl;

  for (auto it = user_params.partition.begin();
       it != user_params.partition.end();
       ++it) {
    PartitionType p_type = it->second.partition_type;
    if (p_type == complete) {
      std::string array_label = it->first;
      unsigned size = it->second.array_size;
      unsigned word_size = it->second.wordsize;
      registers.createRegister(array_label, size, word_size, cycle_time);
    }
  }
}

/*
 * Modify: baseAddress
 */
void ScratchpadDatapath::scratchpadPartition() {
  // read the partition config file to get the address range
  // <base addr, <type, part_factor> >
  if (!user_params.partition.size())
    return;

  std::cout << "-------------------------------" << std::endl;
  std::cout << "      ScratchPad Partition     " << std::endl;
  std::cout << "-------------------------------" << std::endl;
  std::string bn(benchName);

  bool spad_partition = false;
  for (auto part_it = user_params.partition.begin();
       part_it != user_params.partition.end();
       ++part_it) {
    PartitionType p_type = part_it->second.partition_type;
    MemoryType m_type = part_it->second.memory_type;
    if (p_type == complete || m_type != spad)
      continue;
    spad_partition = true;
    std::string array_label = part_it->first;
    Addr base_addr = 0;
    try {
      base_addr = getBaseAddress(array_label);
    } catch (UnknownArrayException& e) {
      std::cerr << "[ERROR]: Could not find the base address of array "
                << array_label << ": " << e.what() << std::endl;
      exit(1);
    }
    unsigned size = part_it->second.array_size;  // num of bytes
    unsigned p_factor = part_it->second.part_factor;
    unsigned wordsize = part_it->second.wordsize;  // in bytes

    PartitionType part_type = cyclic;
    if (p_type == block)
      part_type = block;

    if (!scratchpad_partition_executed) {
      // Only create the scratchpads once.
      scratchpad->setScratchpad(
          array_label, base_addr, part_type, p_factor, size, wordsize);
    } else {
      // On subsequent invocations, just update the base address.
      scratchpad->setLogicalArrayBaseAddress(array_label, base_addr);
    }
  }
  if (!spad_partition)
    return;
  scratchpad_partition_executed = true;

  for (auto node_it = program.nodes.begin(); node_it != program.nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (!node->is_memory_op())
      continue;
    if (boost::degree(node->get_vertex(), program.graph) == 0)
      continue;
    const std::string& base_label = node->get_array_label();

    auto part_it = user_params.partition.find(base_label);
    if (part_it != user_params.partition.end()) {
      PartitionType p_type = part_it->second.partition_type;
      MemoryType m_type = part_it->second.memory_type;
      /* continue if it's complete partition, cache, or acp. */
      if (p_type == complete || m_type != spad)
        continue;
      assert(p_type == block || p_type == cyclic);

      MemAccess* mem_access = node->get_mem_access();
      Addr abs_addr = mem_access->vaddr;
      unsigned data_size = mem_access->size;  // in bytes
      assert(data_size != 0 && "Memory access size must be >= 1 byte.");
      try {
        node->set_partition_index(
            scratchpad->getPartitionIndex(base_label, abs_addr));
      } catch (UnknownArrayException& e) {
        std::cerr << "[ERROR]: At node " << node->get_node_id()
                  << ", could not set partition index on array \"" << base_label
                  << "\" because it does not exist!" << std::endl;
        exit(1);
      } catch (ArrayAccessException& e) {
        std::cerr << "[ERROR]: At node " << node->get_node_id()
                  << ", invalid array access: " << e.what() << std::endl;
        exit(1);
      }
    }
  }
#ifdef DEBUG
  writeBaseAddress();
#endif
}

bool ScratchpadDatapath::step() {
  if (!BaseDatapath::step()) {
    scratchpad->step();
    scratchpadCanService = true;
    return false;
  } else {
    return true;
  }
}

void ScratchpadDatapath::stepExecutingQueue() {
  auto it = executingQueue.begin();
  int index = 0;
  while (it != executingQueue.end()) {
    ExecNode* node = *it;
    bool executed = false;
    if (node->is_memory_op()) {
      const std::string& array_name = node->get_array_label();
      unsigned array_name_index = node->get_partition_index();
      if (registers.has(array_name)) {
        markNodeStarted(node);
        if (node->is_load_op())
          registers.getRegister(array_name)->increment_loads();
        else
          registers.getRegister(array_name)->increment_stores();
        markNodeCompleted(it, index);
        executed = true;
      } else if (scratchpadCanService) {
        PartitionEntry partition = user_params.partition.at(array_name);
        assert(partition.memory_type != dma &&
               "Host memory accesses are not supported by standalone Aladdin!");
        MemAccess* mem_access = node->get_mem_access();
        Addr vaddr = mem_access->vaddr;
        bool isLoad = node->is_load_op();
        try {
          if (scratchpad->canServicePartition(
                  array_name, array_name_index, vaddr, isLoad)) {
            markNodeStarted(node);
            if (isLoad)
              scratchpad->increment_loads(array_name, array_name_index);
            else
              scratchpad->increment_stores(array_name, array_name_index);
            markNodeCompleted(it, index);
            executed = true;
          } else {
            scratchpadCanService = scratchpad->canService();
          }
        } catch (UnknownArrayException& e) {
          std::cerr << "[ERROR]: Node " << node->get_node_id()
                    << " tried to access array \"" << array_name
                    << "\", which does not exist: " << e.what() << std::endl;
          exit(1);
        }
      }
    } else if (node->is_multicycle_op()) {
      unsigned node_id = node->get_node_id();
      if (inflight_multicycle_nodes.find(node_id) ==
          inflight_multicycle_nodes.end()) {
        inflight_multicycle_nodes[node_id] = node->get_multicycle_latency();
        markNodeStarted(node);
      } else {
        unsigned remaining_cycles = inflight_multicycle_nodes.at(node_id);
        if (remaining_cycles == 1) {
          inflight_multicycle_nodes.erase(node_id);
          markNodeCompleted(it, index);
          executed = true;
        } else {
          inflight_multicycle_nodes.at(node_id)--;
        }
      }
    } else {
      markNodeStarted(node);
      markNodeCompleted(it, index);
      executed = true;
    }
    if (!executed) {
      ++it;
      ++index;
    }
  }
}

int ScratchpadDatapath::rescheduleNodesWhenNeeded() {
  std::vector<Vertex> topo_nodes;
  boost::topological_sort(program.graph, std::back_inserter(topo_nodes));
  // bottom nodes first
  std::map<unsigned, int> alap_finish_time;
  for (auto node_it : program.nodes)
    alap_finish_time[node_it.first] = ceil(num_cycles * (double)cycle_time);

  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi) {
    unsigned node_id = program.atVertex(*vi);
    ExecNode* node = program.nodes.at(node_id);
    if (node->is_isolated())
      continue;
    int alap_start_execution_time =
        node->get_start_execution_cycle() * cycle_time;
    /* Do not reschedule memory ops and branch ops.*/
    if (!node->is_memory_op() && !node->is_branch_op()) {
      int alap_complete_execution_time = alap_finish_time.at(node_id);
      int new_cycle =
          floor(alap_complete_execution_time / (double)cycle_time) - 1;
      if (new_cycle > node->get_complete_execution_cycle()) {
        node->set_complete_execution_cycle(new_cycle);
        if (node->is_fp_op()) {
          node->set_start_execution_cycle(
              new_cycle - node->fp_node_latency_in_cycles() + 1);
          alap_start_execution_time =
              node->get_start_execution_cycle() * cycle_time;
        } else {
          node->set_start_execution_cycle(new_cycle);
          alap_start_execution_time =
              alap_complete_execution_time - node->fu_node_latency(cycle_time);
        }
      }
    }

    in_edge_iter in_i, in_end;
    for (boost::tie(in_i, in_end) = in_edges(*vi, program.graph); in_i != in_end;
         ++in_i) {
      int parent_id = program.atVertex(source(*in_i, program.graph));
      if (alap_finish_time.at(parent_id) > alap_start_execution_time)
        alap_finish_time.at(parent_id) = alap_start_execution_time;
    }
  }
  return num_cycles;
}
/* Insert children nodes to the executing queue if they are ready, otherwise
 * decrement children's number of parents executed counters.

 * The difference compared to the BaseDatapath::updateChildren() is that this
 * implementation considers functional units packing, where multiple functional
 * units can be packed into the same cycles if their overall critical path delay
 * is smaller than cycle_time.

 * One thing here is that we only pack multiple functional units into one cycle,
 * but not pack memory operations and functional units together, since memory
 * operation latency is non deterministic, especially in the case of cache
 * access.*/
void ScratchpadDatapath::updateChildren(ExecNode* node) {
  if (!node->has_vertex())
    return;
  float latency_after_current_node = 0;
  if (node->is_memory_op() || node->is_fp_op()) {
    /*No packing for both memory ops and floating point ops. Children can only
     * start at the cycle after. */
    latency_after_current_node = (num_cycles + 1) * cycle_time;
  } else {
    if (node->get_time_before_execution() > num_cycles * cycle_time) {
      latency_after_current_node =
          node->fu_node_latency(cycle_time) + node->get_time_before_execution();
    } else {
      latency_after_current_node =
          node->fu_node_latency(cycle_time) + num_cycles * cycle_time;
    }
  }
  Vertex curr_vertex = node->get_vertex();
  out_edge_iter out_edge_it, out_edge_end;
  for (boost::tie(out_edge_it, out_edge_end) = out_edges(curr_vertex, program.graph);
       out_edge_it != out_edge_end;
       ++out_edge_it) {
    Vertex child_vertex = target(*out_edge_it, program.graph);
    ExecNode* child_node = program.nodeAtVertex(child_vertex);
    int edge_parid = edgeToParid[*out_edge_it];
    float child_earliest_time = child_node->get_time_before_execution();
    if (child_earliest_time < latency_after_current_node) {
      child_node->set_time_before_execution(latency_after_current_node);
    }
    if (child_node->get_num_parents() > 0) {
      child_node->decr_num_parents();
      if (child_node->get_num_parents() == 0) {
        bool child_zero_latency =
            (child_node->is_memory_op())
                ? false
                : (child_node->fu_node_latency(cycle_time) == 0);
        bool curr_zero_latency = (node->is_memory_op())
                                     ? false
                                     : (node->fu_node_latency(cycle_time) == 0);
        if (edge_parid == REGISTER_EDGE || edge_parid == FUSED_BRANCH_EDGE ||
            node->is_call_op() || node->is_ret_op() ||
            ((child_zero_latency || curr_zero_latency) &&
             edge_parid != CONTROL_EDGE)) {
          executingQueue.push_back(child_node);
        } else if (child_node->is_memory_op() || node->is_memory_op() ||
                   child_node->is_fp_op() || node->is_fp_op()) {
          /* Do not pack memory operations and floating point functional units
           *  with others. */
          if (child_node->is_store_op())
            readyToExecuteQueue.push_front(child_node);
          else
            readyToExecuteQueue.push_back(child_node);
        } else {
          /* Both curr node and child node are non-memory, non-fp operations.*/
          if (edge_parid == CONTROL_EDGE) {
            readyToExecuteQueue.push_back(child_node);
          } else {
            float after_child_time = child_node->get_time_before_execution() +
                                     child_node->fu_node_latency(cycle_time);
            if (after_child_time < (num_cycles + 1) * cycle_time)
              executingQueue.push_back(child_node);
            else
              readyToExecuteQueue.push_back(child_node);
          }
        }
        child_node->set_num_parents(-1);
      }
    }
  }
}

#ifdef USE_DB
int ScratchpadDatapath::writeConfiguration(sql::Connection* con) {
  int unrolling_factor, partition_factor;
  bool pipelining_factor;
  getCommonConfigParameters(
      unrolling_factor, pipelining_factor, partition_factor);

  sql::Statement* stmt = con->createStatement();
  stringstream query;
  query << "insert into configs (id, memory_type, trace_file, "
           "config_file, pipelining, unrolling, partitioning) values (";
  query << "NULL"
        << ",\"spad\""
        << ","
        << "\"" << trace_file << "\""
        << ",\"" << config_file << "\"," << pipelining_factor << ","
        << unrolling_factor << "," << partition_factor << ")";
  stmt->execute(query.str());
  delete stmt;
  // Get the newly added config_id.
  return getLastInsertId(con);
}
#endif

double ScratchpadDatapath::getTotalMemArea() {
  return scratchpad->getTotalArea();
}

void ScratchpadDatapath::getMemoryBlocks(std::vector<std::string>& names) {
  scratchpad->getMemoryBlocks(names);
}

void ScratchpadDatapath::getAverageMemPower(unsigned int cycles,
                                            float* avg_power,
                                            float* avg_dynamic,
                                            float* avg_leak) {
  scratchpad->getAveragePower(cycles, avg_power, avg_dynamic, avg_leak);
}

void ScratchpadDatapath::writeOtherStats() {
  std::ofstream spad_stats_file;
  std::string file_name = benchName + "_spad_stats.txt";
  spad_stats_file.open(file_name.c_str(), std::ofstream::out | std::ofstream::app);
  scratchpad->dumpStats(spad_stats_file);
  spad_stats_file.close();
}
