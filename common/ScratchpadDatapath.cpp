/* Implementation of an accelerator datapath that uses a private scratchpad for
 * local memory.
 */

#include <string>

#include "DatabaseDeps.h"
#include "ScratchpadDatapath.h"

ScratchpadDatapath::ScratchpadDatapath(std::string bench,
                                       std::string trace_file,
                                       std::string config_file)
    : BaseDatapath(bench, trace_file, config_file) {
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "      Setting ScratchPad       " << std::endl;
  std::cerr << "-------------------------------" << std::endl;
  scratchpad = new Scratchpad(1, cycleTime);
  scratchpadCanService = true;
}

ScratchpadDatapath::~ScratchpadDatapath() { delete scratchpad; }

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
  std::unordered_map<std::string, unsigned> comp_part_config;
  readCompletePartitionConfig(comp_part_config);
  std::unordered_map<std::string, partitionEntry> part_config;
  readPartitionConfig(part_config);

  for (auto it = exec_nodes.begin(); it != exec_nodes.end(); ++it) {
    BaseNode* node = it->second;
    if (!node->is_memory_op())
      continue;
    std::string part_name = node->get_array_label();
    if (part_config.find(part_name) == part_config.end() &&
        comp_part_config.find(part_name) == comp_part_config.end()) {
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
  std::unordered_map<std::string, unsigned> comp_part_config;
  if (!readCompletePartitionConfig(comp_part_config))
    return;

  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "        Mem to Reg Conv        " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  for (auto it = comp_part_config.begin(); it != comp_part_config.end(); ++it) {
    std::string base_addr = it->first;
    unsigned size = it->second;

    registers.createRegister(base_addr, size, cycleTime);
  }
}

/*
 * Modify: baseAddress
 */
void ScratchpadDatapath::scratchpadPartition() {
  // read the partition config file to get the address range
  // <base addr, <type, part_factor> >
  std::unordered_map<std::string, partitionEntry> part_config;
  if (!readPartitionConfig(part_config))
    return;

  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "      ScratchPad Partition     " << std::endl;
  std::cerr << "-------------------------------" << std::endl;
  std::string bn(benchName);

  // TODO: Should store this along with the node.
  std::unordered_map<unsigned, MemAccess> address;
  initAddress(address);
  // set scratchpad
  for (auto it = part_config.begin(); it != part_config.end(); ++it) {
    std::string base_addr = it->first;
    unsigned size = it->second.array_size;  // num of bytes
    unsigned p_factor = it->second.part_factor;
    unsigned wordsize = it->second.wordsize;  // in bytes
    unsigned per_size = ceil(((float)size) / p_factor);

    for (unsigned i = 0; i < p_factor; i++) {
      ostringstream oss;
      oss << base_addr << "-" << i;
      scratchpad->setScratchpad(oss.str(), per_size, wordsize);
    }
  }

  for (auto node_it = exec_nodes.begin(); node_it != exec_nodes.end();
       ++node_it) {
    BaseNode* node = node_it->second;
    if (!node->is_memory_op())
      continue;
    if (boost::degree(node->get_vertex(), graph_) == 0)
      continue;
    std::string base_label = node->get_array_label();
    long long int base_addr = arrayBaseAddress[base_label];

    auto part_it = part_config.find(base_label);
    if (part_it != part_config.end()) {
      std::string p_type = part_it->second.type;
      assert((!p_type.compare("block")) || (!p_type.compare("cyclic")));

      unsigned num_of_elements = part_it->second.array_size;
      unsigned p_factor = part_it->second.part_factor;
      long long int abs_addr = address[node->get_node_id()].vaddr;
      unsigned data_size = address[node->get_node_id()].size / 8;  // in bytes
      unsigned rel_addr = (abs_addr - base_addr) / data_size;
      if (!p_type.compare("block"))  // block partition
      {
        ostringstream oss;
        unsigned num_of_elements_in_2 = next_power_of_two(num_of_elements);
        oss << base_label << "-"
            << (int)(rel_addr / ceil(num_of_elements_in_2 / p_factor));
        node->set_array_label(oss.str());
      } else  // cyclic partition
      {
        ostringstream oss;
        oss << base_label << "-" << (rel_addr) % p_factor;
        node->set_array_label(oss.str());
      }
    }
  }
  writeBaseAddress();
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
    BaseNode* node = *it;
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

void ScratchpadDatapath::updateChildren(BaseNode* node) {
  if (!node->has_vertex())
    return;
  float latency_after_current_node = node->node_latency();
  unsigned node_id = node->get_node_id();
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
    BaseNode* child_node = getNodeFromVertex(child_vertex);
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
          float after_child_time =
              child_node->get_time_before_execution() + node->node_latency();
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
  bool pipelining;
  getCommonConfigParameters(unrolling_factor, pipelining, partition_factor);

  sql::Statement* stmt = con->createStatement();
  stringstream query;
  query << "insert into configs (id, memory_type, trace_file, "
           "config_file, pipelining, unrolling, partitioning) values (";
  query << "NULL"
        << ",\"spad\""
        << ","
        << "\"" << trace_file << "\""
        << ",\"" << config_file << "\"," << pipelining << ","
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
