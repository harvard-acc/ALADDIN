/* Implementation of an accelerator datapath that uses a private scratchpad for
 * local memory.
 */

#include <string>

#include "ScratchpadDatapath.h"

ScratchpadDatapath::ScratchpadDatapath(std::string bench,
                                       std::string trace_file,
                                       std::string config_file,
                                       int num_spad_ports)
    : BaseDatapath(bench, trace_file, config_file) {
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "      Setting ScratchPad       " << std::endl;
  std::cerr << "-------------------------------" << std::endl;
  scratchpad = new Scratchpad(num_spad_ports, cycleTime);
  scratchpadCanService = true;
}

ScratchpadDatapath::~ScratchpadDatapath() {
  delete scratchpad;
}

void ScratchpadDatapath::globalOptimizationPass() {
  // Node removals must come first.
  removePhiNodes();
  /* memoryAmbiguation() should execute after removeInductionDependence()
   * because it needs induction_nodes. */
  removeInductionDependence();
  memoryAmbiguation();
  // Base address must be initialized next.
  initBaseAddress();
  completePartition();
  scratchpadPartition();
  loopFlatten();
  loopUnrolling();
  removeSharedLoads();
  storeBuffer();
  removeRepeatedStores();
  treeHeightReduction();
  // Must do loop pipelining last; after all the data/control dependences are
  // fixed
  loopPipelining();
}

/* First, compute all base addresses, then check each node to make sure that
 * each entry is valid.
 */
void ScratchpadDatapath::initBaseAddress() {
  BaseDatapath::initBaseAddress();

  for (auto it = exec_nodes.begin(); it != exec_nodes.end(); ++it) {
    ExecNode* node = it->second;
    if (!node->is_memory_op())
      continue;
    std::string part_name = node->get_array_label();
    if (partition_config.find(part_name) == partition_config.end()) {
      std::cerr << "Unknown partition : " << part_name
                << "@inst: " << node->get_node_id() << std::endl;
      exit(-1);
    }
  }
}

/*
 * Modify scratchpad
 */
void ScratchpadDatapath::completePartition() {
  if (!partition_config.size())
    return;

  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "        Mem to Reg Conv        " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  for (auto it = partition_config.begin(); it != partition_config.end(); ++it) {
    std::string array_label = it->first;
    unsigned p_factor = it->second.part_factor;
    /*Complete partition if p_factor is zero*/
    if (p_factor == 0) {
      unsigned size = it->second.array_size;
      registers.createRegister(array_label, size, cycleTime);
    }
  }
}

/*
 * Modify: baseAddress
 */
void ScratchpadDatapath::scratchpadPartition() {
  // read the partition config file to get the address range
  // <base addr, <type, part_factor> >
  if (!partition_config.size())
    return;

  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "      ScratchPad Partition     " << std::endl;
  std::cerr << "-------------------------------" << std::endl;
  std::string bn(benchName);

  // set scratchpad
  for (auto it = partition_config.begin(); it != partition_config.end(); ++it) {
    std::string array_label = it->first;
    unsigned size = it->second.array_size;  // num of bytes
    unsigned p_factor = it->second.part_factor;
    unsigned wordsize = it->second.wordsize;  // in bytes
    unsigned per_size = ceil(((float)size) / p_factor);

    for (unsigned i = 0; i < p_factor; i++) {
      ostringstream oss;
      oss << array_label << "-" << i;
      scratchpad->setScratchpad(oss.str(), per_size, wordsize);
    }
  }

  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (!node->is_memory_op())
      continue;
    if (boost::degree(node->get_vertex(), graph_) == 0)
      continue;
    std::string base_label = node->get_array_label();
    long long int base_addr = getBaseAddress(base_label);

    auto part_it = partition_config.find(base_label);
    if (part_it != partition_config.end()) {
      std::string p_type = part_it->second.type;
      unsigned p_factor = part_it->second.part_factor;
      /* continue if it's complete partition*/
      if (p_factor == 0)
        continue;
      assert((!p_type.compare("block")) || (!p_type.compare("cyclic")));

      unsigned num_of_elements = part_it->second.array_size;
      MemAccess* mem_access = node->get_mem_access();
      long long int abs_addr = mem_access->vaddr;
      unsigned data_size = (mem_access->size) / 8;  // in bytes
      assert(data_size != 0 && "Memory access size must be >= 1 byte.");
      unsigned rel_addr = (abs_addr - base_addr) / data_size;
      if (!p_type.compare("block")) {
        /* block partition */
        ostringstream oss;
        unsigned num_of_elements_in_2 = next_power_of_two(num_of_elements);
        oss << base_label << "-"
            << (int)(rel_addr / ceil(num_of_elements_in_2 / p_factor));
        node->set_array_label(oss.str());
      } else {
        /* cyclic partition */
        ostringstream oss;
        oss << base_label << "-" << (rel_addr) % p_factor;
        node->set_array_label(oss.str());
      }
    }
  }
#ifdef DEBUG
  writeBaseAddress();
#endif
}

void ScratchpadDatapath::setGraphForStepping() {
  BaseDatapath::setGraphForStepping();
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
    if (node->is_memory_op()) {
      std::string node_part = node->get_array_label();
      if (registers.has(node_part)) {
        if (node->is_load_op())
          registers.getRegister(node_part)->increment_loads();
        else
          registers.getRegister(node_part)->increment_stores();
        markNodeCompleted(it, index);
      } else if (scratchpadCanService) {
        if (scratchpad->canServicePartition(node_part)) {
          if (node->is_load_op())
            scratchpad->increment_loads(node_part);
          else
            scratchpad->increment_stores(node_part);
          markNodeCompleted(it, index);
        } else {
          scratchpadCanService = scratchpad->canService();
          ++it;
          ++index;
        }
      } else {
        ++it;
        ++index;
      }
    } else {
      markNodeCompleted(it, index);
    }
  }
}

int ScratchpadDatapath::rescheduleNodesWhenNeeded() {
  std::vector<Vertex> topo_nodes;
  boost::topological_sort(graph_, std::back_inserter(topo_nodes));
  // bottom nodes first
  std::vector<float> alap_finish_time(numTotalNodes, num_cycles * cycleTime);
  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi) {
    unsigned node_id = vertexToName[*vi];
    ExecNode* node = exec_nodes[node_id];
    if (node->is_isolated())
      continue;
    float alap_executing_time = alap_finish_time.at(node_id);
    if (!node->is_memory_op() && !node->is_branch_op()) {
      alap_executing_time -= node->node_latency();
      if (alap_executing_time > (node->get_execution_cycle() + 1) * cycleTime) {
        node->set_execution_cycle(floor(alap_executing_time / cycleTime));
      }
    } else {
      alap_executing_time = node->get_execution_cycle() * cycleTime;
    }

    in_edge_iter in_i, in_end;
    for (tie(in_i, in_end) = in_edges(*vi, graph_); in_i != in_end; ++in_i) {
      int parent_id = vertexToName[source(*in_i, graph_)];
      if (alap_finish_time.at(parent_id) > alap_executing_time)
        alap_finish_time.at(parent_id) = alap_executing_time;
    }
  }
  return num_cycles;
}
void ScratchpadDatapath::updateChildren(ExecNode* node) {
  if (!node->has_vertex())
    return;
  float latency_after_current_node = node->node_latency();
  if (node->get_time_before_execution() > num_cycles * cycleTime) {
    latency_after_current_node += node->get_time_before_execution();
  } else {
    latency_after_current_node += num_cycles * cycleTime;
  }
  Vertex v = node->get_vertex();
  out_edge_iter out_edge_it, out_edge_end;
  for (tie(out_edge_it, out_edge_end) = out_edges(v, graph_);
       out_edge_it != out_edge_end;
       ++out_edge_it) {
    Vertex child_vertex = target(*out_edge_it, graph_);
    ExecNode* child_node = getNodeFromVertex(child_vertex);
    int edge_parid = edgeToParid[*out_edge_it];
    float child_earliest_time = child_node->get_time_before_execution();
    if (edge_parid != CONTROL_EDGE &&
        child_earliest_time < latency_after_current_node)
      child_node->set_time_before_execution(latency_after_current_node);
    if (child_node->get_num_parents() > 0) {
      child_node->decr_num_parents();
      if (child_node->get_num_parents() == 0) {
        if ((child_node->node_latency() == 0 || node->node_latency() == 0) &&
            edge_parid != CONTROL_EDGE)
          executingQueue.push_back(child_node);
        else if (child_node->is_memory_op()) {
          if (child_node->is_store_op())
            readyToExecuteQueue.push_front(child_node);
          else
            readyToExecuteQueue.push_back(child_node);
        } else {
          float after_child_time = child_node->get_time_before_execution() +
                                   child_node->node_latency();
          if (after_child_time < (num_cycles + 1) * cycleTime &&
              edge_parid != CONTROL_EDGE)
            executingQueue.push_back(child_node);
          else
            readyToExecuteQueue.push_back(child_node);
        }
        child_node->set_num_parents(-1);
      }
    }
  }
}

void ScratchpadDatapath::dumpStats() {
  BaseDatapath::dumpStats();
  BaseDatapath::writePerCycleActivity();
}

#ifdef USE_DB
int ScratchpadDatapath::writeConfiguration(sql::Connection* con) {
  int unrolling_factor, partition_factor;
  bool pipelining_factor;
  getCommonConfigParameters(unrolling_factor, pipelining_factor, partition_factor);

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

unsigned ScratchpadDatapath::getTotalMemSize() {
  return scratchpad->getTotalSize();
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
