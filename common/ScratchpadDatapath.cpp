/* Implementation of an accelerator datapath that uses a private scratchpad for
 * local memory.
 */

#include <string>

#include "ScratchpadDatapath.h"

ScratchpadDatapath::ScratchpadDatapath(
    std::string bench, std::string trace_file,
    std::string config_file, float cycle_t) :
    BaseDatapath(bench, trace_file, config_file, cycle_t) {}

ScratchpadDatapath::~ScratchpadDatapath() {}

void ScratchpadDatapath::setScratchpad(Scratchpad *spad)
{
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "      Setting ScratchPad       " << std::endl;
  std::cerr << "-------------------------------" << std::endl;
  scratchpad = spad;
}

void ScratchpadDatapath::globalOptimizationPass() {
  // Node removals must come first.
  removeInductionDependence();
  removePhiNodes();
  // Base address must be initialized next.
  initBaseAddress();
  completePartition();
  scratchpadPartition();
  loopFlatten();
  loopUnrolling();
  memoryAmbiguation();
  removeSharedLoads();
  storeBuffer();
  removeRepeatedStores();
  treeHeightReduction();
  // Must do loop pipelining last; after all the data/control dependences are fixed
  loopPipelining();
}
/*
 * Read: graph, getElementPtr.gz, completePartitionConfig, PartitionConfig
 * Modify: baseAddress
 */
void ScratchpadDatapath::initBaseAddress()
{
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "       Init Base Address       " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  std::unordered_map<std::string, unsigned> comp_part_config;
  readCompletePartitionConfig(comp_part_config);
  std::unordered_map<std::string, partitionEntry> part_config;
  readPartitionConfig(part_config);

  std::unordered_map<unsigned, pair<std::string, long long int> > getElementPtr;
  initGetElementPtr(getElementPtr);

  edgeToParid = get(boost::edge_name, graph_);

  vertex_iter vi, vi_end;
  for (tie(vi, vi_end) = vertices(graph_); vi != vi_end; ++vi)
  {
    if (boost::degree(*vi, graph_) == 0)
      continue;
    Vertex curr_node = *vi;
    unsigned node_id = vertexToName[curr_node];
    int node_microop = microop.at(node_id);
    if (!is_memory_op(node_microop))
      continue;
    bool modified = 0;
    //iterate its parents, until it finds the root parent
    while (true) {
      bool found_parent = 0;
      in_edge_iter in_edge_it, in_edge_end;

      for (tie(in_edge_it, in_edge_end) = in_edges(curr_node , graph_);
                             in_edge_it != in_edge_end; ++in_edge_it) {
        int edge_parid = edgeToParid[*in_edge_it];
        if ( ( node_microop == LLVM_IR_Load && edge_parid != 1 )
             || ( node_microop == LLVM_IR_GetElementPtr && edge_parid != 1 )
             || ( node_microop == LLVM_IR_Store && edge_parid != 2) )
          continue;

        unsigned parent_id = vertexToName[source(*in_edge_it, graph_)];
        int parent_microop = microop.at(parent_id);
        if (parent_microop == LLVM_IR_GetElementPtr
            || parent_microop == LLVM_IR_Load) {
          //remove address calculation directly
          baseAddress[node_id] = getElementPtr[parent_id];
          curr_node = source(*in_edge_it, graph_);
          node_microop = parent_microop;
          found_parent = 1;
          modified = 1;
          break;
        }
        else if (parent_microop == LLVM_IR_Alloca) {
          std::string part_name = getElementPtr[parent_id].first;
          baseAddress[node_id] = getElementPtr[parent_id];
          modified = 1;
          break;
        }
      }
      if (!found_parent)
        break;
    }
    if (!modified)
      baseAddress[node_id] = getElementPtr[node_id];

    std::string part_name = baseAddress[node_id].first;
    if (part_config.find(part_name) == part_config.end() &&
          comp_part_config.find(part_name) == comp_part_config.end() ) {
      std::cerr << "Unknown partition : " << part_name << "@inst: "
                << node_id << std::endl;
      exit(-1);
    }
  }
  writeBaseAddress();
}

/*
 * Modify scratchpad
 */
void ScratchpadDatapath::completePartition()
{
  std::unordered_map<std::string, unsigned> comp_part_config;
  if (!readCompletePartitionConfig(comp_part_config))
    return;

  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "        Mem to Reg Conv        " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  for (auto it = comp_part_config.begin(); it != comp_part_config.end(); ++it)
  {
    std::string base_addr = it->first;
    unsigned size = it->second;

    registers.createRegister(base_addr, size, cycleTime);
  }
}

/*
 * Modify: baseAddress
 */
void ScratchpadDatapath::scratchpadPartition()
{
  //read the partition config file to get the address range
  // <base addr, <type, part_factor> >
  std::unordered_map<std::string, partitionEntry> part_config;
  if (!readPartitionConfig(part_config))
    return;

  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "      ScratchPad Partition     " << std::endl;
  std::cerr << "-------------------------------" << std::endl;
  std::string bn(benchName);

  std::unordered_map<unsigned, pair<long long int, unsigned> > address;
  initAddressAndSize(address);
  //set scratchpad
  for(auto it = part_config.begin(); it!= part_config.end(); ++it)
  {
    std::string base_addr = it->first;
    unsigned size = it->second.array_size; //num of bytes
    unsigned p_factor = it->second.part_factor;
    unsigned wordsize = it->second.wordsize; //in bytes
    unsigned per_size = ceil( ((float)size) / p_factor);

    for ( unsigned i = 0; i < p_factor ; i++)
    {
      ostringstream oss;
      oss << base_addr << "-" << i;
      scratchpad->setScratchpad(oss.str(), per_size, wordsize);
    }
  }

  for(unsigned node_id = 0; node_id < numTotalNodes; node_id++)
  {
    if (!is_memory_op(microop.at(node_id)))
      continue;

    if (baseAddress.find(node_id) == baseAddress.end())
      continue;
    std::string base_label  = baseAddress[node_id].first;
    long long int base_addr = baseAddress[node_id].second;

    auto part_it = part_config.find(base_label);
    if (part_it != part_config.end())
    {
      std::string p_type = part_it->second.type;
      assert((!p_type.compare("block")) || (!p_type.compare("cyclic")));

      unsigned num_of_elements = part_it->second.array_size;
      unsigned p_factor        = part_it->second.part_factor;
      long long int abs_addr   = address[node_id].first;
      unsigned data_size       = address[node_id].second / 8; //in bytes
      unsigned rel_addr        = (abs_addr - base_addr ) / data_size;
      if (!p_type.compare("block"))  //block partition
      {
        ostringstream oss;
        unsigned num_of_elements_in_2 = next_power_of_two(num_of_elements);
        oss << base_label << "-"
            << (int) (rel_addr / ceil (num_of_elements_in_2  / p_factor));
        baseAddress[node_id].first = oss.str();
      }
      else // cyclic partition
      {
        ostringstream oss;
        oss << base_label << "-" << (rel_addr) % p_factor;
        baseAddress[node_id].first = oss.str();
      }
    }
  }
  writeBaseAddress();
}

bool ScratchpadDatapath::step() {
  return BaseDatapath::step();
}

void ScratchpadDatapath::stepExecutingQueue()
{
  auto it = executingQueue.begin();
  int index = 0;
  while (it != executingQueue.end())
  {
    unsigned node_id = *it;
    if (is_memory_op(microop.at(node_id)))
    {
      std::string node_part = baseAddress[node_id].first;
      if (registers.has(node_part) ||
          scratchpad->canServicePartition(node_part))
      {
        if (registers.has(node_part))
        {
          if (is_load_op(microop.at(node_id)))
            registers.getRegister(node_part)->increment_loads();
          else
            registers.getRegister(node_part)->increment_stores();
        }
        else
        {
          assert(scratchpad->addressRequest(node_part));
          if (is_load_op(microop.at(node_id)))
            scratchpad->increment_loads(node_part);
          else
            scratchpad->increment_stores(node_part);
        }
        markNodeCompleted(it, index);
      }
      else
      {
        ++it;
        ++index;
      }
    }
    else
    {
      markNodeCompleted(it, index);
    }
  }
}

void ScratchpadDatapath::dumpStats() {
  BaseDatapath::dumpStats();
  BaseDatapath::writePerCycleActivity();
}

double ScratchpadDatapath::getTotalMemArea()
{
  return scratchpad->getTotalArea();
}

unsigned ScratchpadDatapath::getTotalMemSize()
{
  return scratchpad->getTotalSize();
}

void ScratchpadDatapath::getMemoryBlocks(std::vector<std::string> &names)
{
  scratchpad->getMemoryBlocks(names);
}

void ScratchpadDatapath::getAverageMemPower(
    unsigned int cycles, float *avg_power, float *avg_dynamic, float *avg_leak)
{
  scratchpad->getAveragePower(cycles, avg_power, avg_dynamic, avg_leak);
}
