/* Implementation of an accelerator datapath that uses a private scratchpad for
 * local memory.
 */

#include <string>

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
  /*memoryAmibguation() should execute after removeInductionDependence() because
   * it needs induction_nodes.*/
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

  for (auto it = nodeToLabel.begin(); it != nodeToLabel.end(); ++it) {
    unsigned node_id = it->first;
    std::string part_name = nodeToLabel[node_id];
    if (part_config.find(part_name) == part_config.end() &&
        comp_part_config.find(part_name) == comp_part_config.end()) {
      std::cerr << "Unknown partition : " << part_name << "@inst: " << node_id
                << std::endl;
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

  std::unordered_map<unsigned, MemAccess> address;
  initAddress(address);
  // set scratchpad
  std::unordered_map<unsigned, uca_org_t> cacti_value;
  std::unordered_map<unsigned, uca_org_t>::iterator cacti_it;
  for (auto it = part_config.begin(); it != part_config.end(); ++it) {
    std::string base_addr = it->first;
    unsigned size = it->second.array_size;  // num of bytes
    unsigned p_factor = it->second.part_factor;
    unsigned wordsize = it->second.wordsize;  // in bytes
    unsigned per_size = ceil(((float)size) / p_factor);
    
    uca_org_t cacti_result;
    cacti_it = cacti_value.find(per_size);
    if(cacti_it == cacti_value.end())
    {
        cacti_result = cactiWrapper(per_size, wordsize);  
        cacti_value[per_size] = cacti_result;
    }
    else 
        cacti_result = cacti_it->second;
    for (unsigned i = 0; i < p_factor; i++) {
      ostringstream oss;
      oss << base_addr << "-" << i;
      scratchpad->setScratchpad(oss.str(), per_size, wordsize, cacti_result);
    }
  }

  for (unsigned node_id = 0; node_id < numTotalNodes; node_id++) {
    if (!is_memory_op(microop.at(node_id)))
      continue;

    if (nodeToLabel.find(node_id) == nodeToLabel.end())
      continue;
    std::string base_label = nodeToLabel[node_id];
    long long int base_addr = arrayBaseAddress[base_label];

    auto part_it = part_config.find(base_label);
    if (part_it != part_config.end()) {
      std::string p_type = part_it->second.type;
      assert((!p_type.compare("block")) || (!p_type.compare("cyclic")));

      unsigned num_of_elements = part_it->second.array_size;
      unsigned p_factor = part_it->second.part_factor;
      long long int abs_addr = address[node_id].vaddr;
      unsigned data_size = address[node_id].size / 8;  // in bytes
      unsigned rel_addr = (abs_addr - base_addr) / data_size;
      if (!p_type.compare("block"))  // block partition
      {
        ostringstream oss;
        unsigned num_of_elements_in_2 = next_power_of_two(num_of_elements);
        oss << base_label << "-"
            << (int)(rel_addr / ceil(num_of_elements_in_2 / p_factor));
        nodeToLabel[node_id] = oss.str();
      } else  // cyclic partition
      {
        ostringstream oss;
        oss << base_label << "-" << (rel_addr) % p_factor;
        nodeToLabel[node_id] = oss.str();
      }
    }
  }
  writeBaseAddress();
}

void ScratchpadDatapath::setGraphForStepping() {
  BaseDatapath::setGraphForStepping();
  timeBeforeNodeExecution.assign(numTotalNodes, 0);
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
    unsigned node_id = *it;
    if (is_memory_op(microop.at(node_id))) {
      std::string node_part = nodeToLabel[node_id];
      if (registers.has(node_part)) {
        if (is_load_op(microop.at(node_id)))
          registers.getRegister(node_part)->increment_loads();
        else
          registers.getRegister(node_part)->increment_stores();
        markNodeCompleted(it, index);
      } else if (scratchpadCanService) {
        if (scratchpad->canServicePartition(node_part)) {
          if (is_load_op(microop.at(node_id)))
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
int ScratchpadDatapath::rescheduleNodesWhenNeeded(){
  std::vector<Vertex> topo_nodes;
  boost::topological_sort(graph_, std::back_inserter(topo_nodes));
  // bottom nodes first
  std::vector<float> alap_finish_time(numTotalNodes, num_cycles * cycleTime);
  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi) {
    unsigned node_id = vertexToName[*vi];
    if (finalIsolated.at(node_id))
      continue;
    unsigned node_microop = microop.at(node_id);
    float alap_executing_time = alap_finish_time.at(node_id);
    if (!is_memory_op(node_microop) && !is_branch_op(node_microop)) {
      alap_executing_time -= node_latency(node_microop);
      if (alap_executing_time > (newLevel.at(node_id) + 1) * cycleTime) {
        newLevel.at(node_id) = floor(alap_executing_time / cycleTime);
      }
    } else {
      alap_executing_time = newLevel.at(node_id) * cycleTime;
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
void ScratchpadDatapath::updateChildren(unsigned node_id) {
  if (nameToVertex.find(node_id) == nameToVertex.end())
    return;
  float latency_after_current_node = node_latency(microop.at(node_id));
  if (timeBeforeNodeExecution.at(node_id) > num_cycles * cycleTime) {
    latency_after_current_node += timeBeforeNodeExecution.at(node_id);
  }
  else {
    latency_after_current_node += num_cycles * cycleTime;
  }
  Vertex node = nameToVertex[node_id];
  out_edge_iter out_edge_it, out_edge_end;
  for (tie(out_edge_it, out_edge_end) = out_edges(node, graph_);
       out_edge_it != out_edge_end;
       ++out_edge_it) {
    unsigned child_id = vertexToName[target(*out_edge_it, graph_)];
    int edge_parid = edgeToParid[*out_edge_it];
    float child_earliest_time = timeBeforeNodeExecution.at(child_id);
    if (edge_parid != CONTROL_EDGE && child_earliest_time < latency_after_current_node)
      timeBeforeNodeExecution.at(child_id) = latency_after_current_node;
    if (numParents[child_id] > 0) {
      numParents[child_id]--;
      if (numParents[child_id] == 0) {
        unsigned child_microop = microop.at(child_id);
        if ((node_latency(child_microop) == 0 ||
             node_latency(microop.at(node_id)) == 0) &&
            edge_parid != CONTROL_EDGE)
          executingQueue.push_back(child_id);
        else if (is_memory_op(child_microop)) {
          if (is_store_op(child_microop))
            readyToExecuteQueue.push_front(child_id);
          else
            readyToExecuteQueue.push_back(child_id);
        }
        else {
          float after_child_time = timeBeforeNodeExecution.at(child_id)
            + node_latency(microop.at(child_id));
          if (after_child_time < (num_cycles + 1) * cycleTime &&
              edge_parid != CONTROL_EDGE )
            executingQueue.push_back(child_id);
          else
            readyToExecuteQueue.push_back(child_id);
        }
        numParents[child_id] = -1;
      }
    }
  }
}

void ScratchpadDatapath::dumpStats() {
  BaseDatapath::dumpStats();
  BaseDatapath::writePerCycleActivity();
}

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
uca_org_t ScratchpadDatapath::cactiWrapper(unsigned num_of_bytes, unsigned wordsize) {
  int cache_size = num_of_bytes;
  int line_size = wordsize;  // in bytes
  if (wordsize < 4)          // minimum line size in cacti is 32-bit/4-byte
    line_size = 4;
  if (cache_size / line_size < 64)
    cache_size = line_size * 64;  // minimum scratchpad size: 64 words
  int associativity = 1;
  int rw_ports = RW_PORTS;
  int excl_read_ports = 0;
  int excl_write_ports = 0;
  int single_ended_read_ports = 0;
  int search_ports = 0;
  int banks = 1;
  double tech_node = 40;  // in nm
  //# following three parameters are meaningful only for main memories
  int page_sz = 0;
  int burst_length = 8;
  int pre_width = 8;
  int output_width = wordsize * 8;
  //# to model special structure like branch target buffers, directory, etc.
  //# change the tag size parameter
  //# if you want cacti to calculate the tagbits, set the tag size to "default"
  int specific_tag = false;
  int tag_width = 0;
  int access_mode = 2;  // 0 normal, 1 seq, 2 fast
  int cache = 0;        // scratch ram 0 or cache 1
  int main_mem = 0;
  // assign weights for CACTI optimizations
  int obj_func_delay = 0;
  int obj_func_dynamic_power = 0;
  int obj_func_leakage_power = 1000000;
  int obj_func_area = 0;
  int obj_func_cycle_time = 0;
  // from CACTI example config...
  int dev_func_delay = 100;
  int dev_func_dynamic_power = 1000;
  int dev_func_leakage_power = 10;
  int dev_func_area = 100;
  int dev_func_cycle_time = 100;

  int ed_ed2_none = 2;  // 0 - ED, 1 - ED^2, 2 - use weight and deviate
  int temp = 300;
  int wt = 0;  // 0 - default(search across everything), 1 - global, 2 - 5%
               // delay penalty, 3 - 10%, 4 - 20 %, 5 - 30%, 6 - low-swing
  int data_arr_ram_cell_tech_flavor_in =
      0;  // 0(itrs-hp) 1-itrs-lstp(low standby power)
  int data_arr_peri_global_tech_flavor_in = 0;  // 0(itrs-hp)
  int tag_arr_ram_cell_tech_flavor_in = 2;      // itrs-hp
  int tag_arr_peri_global_tech_flavor_in = 2;   // itrs-hp
  int interconnect_projection_type_in = 1;      // 0 - aggressive, 1 - normal
  int wire_inside_mat_type_in = 2;   // 2 - global, 0 - local, 1 - semi-global
  int wire_outside_mat_type_in = 2;  // 2 - global
  int REPEATERS_IN_HTREE_SEGMENTS_in =
      1;  // TODO for now only wires with repeaters are supported
  int VERTICAL_HTREE_WIRES_OVER_THE_ARRAY_in = 0;
  int BROADCAST_ADDR_DATAIN_OVER_VERTICAL_HTREES_in = 0;
  int force_wiretype = 1;
  int wiretype = 30;
  int force_config = 0;
  int ndwl = 1;
  int ndbl = 1;
  int nspd = 0;
  int ndcm = 1;
  int ndsam1 = 0;
  int ndsam2 = 0;
  int ecc = 0;
  return cacti_interface(cache_size,
                         line_size,
                         associativity,
                         rw_ports,
                         excl_read_ports,
                         excl_write_ports,
                         single_ended_read_ports,
                         search_ports,
                         banks,
                         tech_node,  // in nm
                         output_width,
                         specific_tag,
                         tag_width,
                         access_mode,  // 0 normal, 1 seq, 2 fast
                         cache,        // scratch ram or cache
                         main_mem,
                         obj_func_delay,
                         obj_func_dynamic_power,
                         obj_func_leakage_power,
                         obj_func_area,
                         obj_func_cycle_time,
                         dev_func_delay,
                         dev_func_dynamic_power,
                         dev_func_leakage_power,
                         dev_func_area,
                         dev_func_cycle_time,
                         ed_ed2_none,
                         temp,
                         wt,
                         data_arr_ram_cell_tech_flavor_in,
                         data_arr_peri_global_tech_flavor_in,
                         tag_arr_ram_cell_tech_flavor_in,
                         tag_arr_peri_global_tech_flavor_in,
                         interconnect_projection_type_in,
                         wire_inside_mat_type_in,
                         wire_outside_mat_type_in,
                         REPEATERS_IN_HTREE_SEGMENTS_in,
                         VERTICAL_HTREE_WIRES_OVER_THE_ARRAY_in,
                         BROADCAST_ADDR_DATAIN_OVER_VERTICAL_HTREES_in,
                         page_sz,
                         burst_length,
                         pre_width,
                         force_wiretype,
                         wiretype,
                         force_config,
                         ndwl,
                         ndbl,
                         nspd,
                         ndcm,
                         ndsam1,
                         ndsam2,
                         ecc);
}
