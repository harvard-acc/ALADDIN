#include "./Scratchpad.h"

const unsigned MEM_size[]  = {64, 128, 256, 512, 1024, 2049, 4098, 8196, 16392, 32784, 65568, 131136, 262272};
const float MEM_rd_power[] = {1.779210, 1.779210, 2.653500, 2.653500, 3.569050, 4.695780, 5.883620, 7.587260, 9.458480, 8.363850, 13.472600, 12.640600, 18.336900};
const float MEM_wr_power[] = {1.733467, 1.733467, 2.531965, 2.531965, 3.138079, 3.783919, 4.450720, 5.007659, 5.370660, 4.590109, 7.371770, 5.849070, 6.549049};
const float MEM_lk_power[] = {0.013156, 0.026312, 0.052599, 0.105198, 0.210474, 0.420818, 0.841640, 1.682850, 3.365650, 6.729040, 13.459700, 26.916200, 53.832100};
const float MEM_area[]     = {1616.140000, 2929.000000, 4228.290000, 7935.990000, 15090.200000, 28129.300000, 49709.900000, 94523.900000, 174459.000000, 352194.000000, 684305.000000, 1319220.000000, 2554980.000000};

/*
const float widths[] = {8,12,16,20,24,28,32,34,36,40,45,50,55,60,64};

//Leakage needs to be divided by 100 to be in mW for FUs.
const float MUL_area[] = {301.759902,818.928509,1542.58912,2866.015128,3149.003032,4140.164648,5251.936368,5867.657979,6507.783191,7881.893616,9783.027954,11908.4877,14214.15864,16932.34429,20056.47439};
const float MUL_switching[] = {0.013842,0.090178,0.179611,0.295211,0.207411,0.245411,0.283311,0.302311,0.324411,0.367711,0.422211,0.480311,0.430111,0.530211,0.656111};
const float MUL_internal[] = {0.0092,0.065,0.1745,0.4002,0.3053,0.362,0.4175,0.4459,0.4796,0.545,0.63,0.7185,0.7901,0.9574,0.9601};
const float MUL_leakage[] = {0.12231,0.37755,0.72451,1.59701,1.87651,2.41711,2.98601,3.32441,3.65261,4.37341,5.34041,6.38861,7.58891,8.99281,10.76271};

const float ADD_area[] = {11.7325,42.706301,51.153703,77.434505,103.715306,129.996109,156.27691,182.557712,208.838514,221.978915,235.119315,261.400117,294.25112,327.102122,359.953123,392.804126,665.936713};
const float ADD_switching[] = {0.000245,0.001954,0.002283,0.002621,0.002586,0.002599,0.002653,0.002609,0.002622,0.002632,0.002624,0.002586,0.002614,0.002623,0.002605,0.008592,0.00412};
const float ADD_internal[] = {0.001118,0.003741,0.005385,0.005671,0.005575,0.005637,0.005731,0.005688,0.005645,0.005709,0.005688,0.005623,0.005657,0.00568,0.005651,0.034227,0.00854};
const float ADD_leakage[] = {0.00815,0.01764,0.02342,0.03361,0.0438,0.05398,0.06417,0.07435,0.08455,0.08964,0.09473,0.10491,0.11765,0.13039,0.14312,0.15586,0.39589};

//This is per reg. mult by bit width.
const float REG_area = 7.98;
const float REG_switching = 0.000125;
const float REG_internal = 0.00191;
const float REG_leakage = 0.000055;
*/

Scratchpad::Scratchpad(unsigned p_ports_per_part)
{
  numOfPortsPerPartition = p_ports_per_part;
}
Scratchpad::~Scratchpad()
{}

void Scratchpad::setScratchpad(string baseName, unsigned size)
{
  assert(!partitionExist(baseName));
  //size: number of words
  unsigned new_id = baseToPartitionID.size();
  baseToPartitionID[baseName] = new_id;
  occupiedBWPerPartition.push_back(0);
  sizePerPartition.push_back(size);
  //set
  unsigned mem_size = next_power_of_two(size);
  if (mem_size < 64)
    mem_size = 64;
  unsigned mem_index = (unsigned) log2(mem_size) - 6;
  readPowerPerPartition.push_back(MEM_rd_power[mem_index]);
  writePowerPerPartition.push_back(MEM_wr_power[mem_index]);
  leakPowerPerPartition.push_back(MEM_lk_power[mem_index]);
  areaPerPartition.push_back(MEM_area[mem_index]);
}

void Scratchpad::step()
{
  for (auto it = occupiedBWPerPartition.begin(); it != occupiedBWPerPartition.end(); ++it)
    *it = 0;
}
bool Scratchpad::partitionExist(string baseName)
{
  auto partition_it = baseToPartitionID.find(baseName);
  if (partition_it != baseToPartitionID.end())
    return 1;
  else
    return 0;
}
bool Scratchpad::addressRequest(string baseName)
{
  if (canServicePartition(baseName))
  {
    unsigned partition_id = findPartitionID(baseName);
    assert(occupiedBWPerPartition.at(partition_id) < numOfPortsPerPartition);
    occupiedBWPerPartition.at(partition_id)++;
    return true;
  }
  else
    return false;
}
bool Scratchpad::canService()
{
  for(auto base_it = baseToPartitionID.begin(); base_it != baseToPartitionID.end(); ++base_it)
  {
    string base_name = base_it->first;
    if (canServicePartition(base_name))
      return 1;
  }
  return 0;
}
bool Scratchpad::canServicePartition(string baseName)
{
  unsigned partition_id = findPartitionID(baseName);
  if (occupiedBWPerPartition.at(partition_id) < numOfPortsPerPartition)
    return 1;
  else
    return 0;
}

unsigned Scratchpad::findPartitionID(string baseName)
{
  auto partition_it = baseToPartitionID.find(baseName);
  if (partition_it != baseToPartitionID.end())
    return partition_it->second;
  else
  {
    cerr << "Unknown Partition Name:" << baseName << endl;
    exit(0);
  }
}

void Scratchpad::partitionNames(std::vector<string> &names)
{
  for(auto base_it = baseToPartitionID.begin(); base_it != baseToPartitionID.end(); ++base_it)
  {
    string base_name = base_it->first;
    names.push_back(base_name);
  }
}

float Scratchpad::readPower (std::string baseName)
{
  unsigned partition_id = findPartitionID(baseName);
  return readPowerPerPartition.at(partition_id);
}

float Scratchpad::writePower (std::string baseName)
{
  unsigned partition_id = findPartitionID(baseName);
  return writePowerPerPartition.at(partition_id);
}

float Scratchpad::leakPower (std::string baseName)
{
  unsigned partition_id = findPartitionID(baseName);
  return leakPowerPerPartition.at(partition_id);
}

float Scratchpad::area (std::string baseName)
{
  unsigned partition_id = findPartitionID(baseName);
  return areaPerPartition.at(partition_id);
}
