/* Implementation of an accelerator datapath that uses a private scratchpad for
 * local memory.
 */

#include <string>

#include "ScratchpadDatapath.h"

ScratchpadDatapath::ScratchpadDatapath(
    std::string bench, std::string trace_file,
    std::string config_file):
    BaseDatapath(bench, trace_file, config_file)
{
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "      Setting ScratchPad       " << std::endl;
  std::cerr << "-------------------------------" << std::endl;
  scratchpad = new Scratchpad(1, cycleTime);
}

ScratchpadDatapath::~ScratchpadDatapath()
{
  delete scratchpad;
}

void ScratchpadDatapath::globalOptimizationPass()
{
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

/* First, compute all base addresses, then check each node to make sure that
 * each entry is valid.
 */
void ScratchpadDatapath::initBaseAddress()
{
  BaseDatapath::initBaseAddress();
  std::unordered_map<std::string, unsigned> comp_part_config;
  readCompletePartitionConfig(comp_part_config);
  std::unordered_map<std::string, partitionEntry> part_config;
  readPartitionConfig(part_config);

  for (auto it = nodeToLabel.begin(); it != nodeToLabel.end(); ++it) {
    unsigned node_id = it->first;
    std::string part_name = nodeToLabel[node_id];
    if (part_config.find(part_name) == part_config.end() &&
          comp_part_config.find(part_name) == comp_part_config.end() ) {
      std::cerr << "Unknown partition : " << part_name << "@inst: "
                << node_id << std::endl;
      exit(-1);
    }
  }
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

  std::unordered_map<unsigned, MemAccess> address;
  initAddress(address);
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

    if (nodeToLabel.find(node_id) == nodeToLabel.end())
      continue;
    std::string base_label  = nodeToLabel[node_id];
    long long int base_addr = arrayBaseAddress[base_label];

    auto part_it = part_config.find(base_label);
    if (part_it != part_config.end())
    {
      std::string p_type = part_it->second.type;
      assert((!p_type.compare("block")) || (!p_type.compare("cyclic")));

      unsigned num_of_elements = part_it->second.array_size;
      unsigned p_factor        = part_it->second.part_factor;
      long long int abs_addr   = address[node_id].vaddr;
      unsigned data_size       = address[node_id].size / 8; //in bytes
      unsigned rel_addr        = (abs_addr - base_addr ) / data_size;
      if (!p_type.compare("block"))  //block partition
      {
        ostringstream oss;
        unsigned num_of_elements_in_2 = next_power_of_two(num_of_elements);
        oss << base_label << "-"
            << (int) (rel_addr / ceil (num_of_elements_in_2  / p_factor));
        nodeToLabel[node_id] = oss.str();
      }
      else // cyclic partition
      {
        ostringstream oss;
        oss << base_label << "-" << (rel_addr) % p_factor;
        nodeToLabel[node_id] = oss.str();
      }
    }
  }
  writeBaseAddress();
}

bool ScratchpadDatapath::step() {
  if (!BaseDatapath::step())
  {
    scratchpad->step();
    return false;
  }
  else
  {
    return true;
  }
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
      std::string node_part = nodeToLabel[node_id];
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
