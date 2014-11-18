#include "generic_func.h"
#include "Scratchpad.h"

Scratchpad::Scratchpad(unsigned p_ports_per_part, float cycle_time)
{
  numOfPortsPerPartition = p_ports_per_part;
  cycleTime = cycle_time;
}

Scratchpad::~Scratchpad() {}

void Scratchpad::setCompScratchpad(std::string baseName, unsigned num_of_bytes)
{
  assert(!partitionExist(baseName));
  //size: number of words
  unsigned new_id = baseToPartitionID.size();
  baseToPartitionID[baseName] = new_id;

  compPartition.push_back(true);
  occupiedBWPerPartition.push_back(0);
  sizePerPartition.push_back(num_of_bytes);
  //set
  // The correct way to do this is to find the read/write energy of register,
  // todo
  readEnergyPerPartition.push_back(num_of_bytes * 8 * (REG_int_power + REG_sw_power)* cycleTime);
  writeEnergyPerPartition.push_back(num_of_bytes * 8 * (REG_int_power + REG_sw_power)* cycleTime);
  leakPowerPerPartition.push_back(num_of_bytes * 8 * REG_leak_power);
  areaPerPartition.push_back(num_of_bytes * 8 * REG_area);
}
void Scratchpad::setScratchpad(std::string baseName, unsigned num_of_bytes, unsigned wordsize)
//wordsize in bytes
{
  assert(!partitionExist(baseName));
  //size: number of words
  unsigned new_id = baseToPartitionID.size();
  baseToPartitionID[baseName] = new_id;

  compPartition.push_back(false);
  occupiedBWPerPartition.push_back(0);
  sizePerPartition.push_back(num_of_bytes);
  //set read/write/leak/area per partition
  uca_org_t cacti_result = cactiWrapper(num_of_bytes, wordsize);

  //power in mW, energy in nJ, area in mm2
  readEnergyPerPartition.push_back(cacti_result.power.readOp.dynamic*1e+9);
  writeEnergyPerPartition.push_back(cacti_result.power.writeOp.dynamic*1e+9);
  leakPowerPerPartition.push_back(cacti_result.power.readOp.leakage*1000);
  areaPerPartition.push_back(cacti_result.area);
}

void Scratchpad::step()
{
  for (auto it = occupiedBWPerPartition.begin(); it != occupiedBWPerPartition.end(); ++it){
    *it = 0;
}
}
bool Scratchpad::partitionExist(std::string baseName)
{
  auto partition_it = baseToPartitionID.find(baseName);
  if (partition_it != baseToPartitionID.end())
    return 1;
  else
    return 0;
}
bool Scratchpad::addressRequest(std::string baseName)
{
  if (canServicePartition(baseName))
  {
    unsigned partition_id = findPartitionID(baseName);
    if (compPartition.at(partition_id))
      return true;
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
    std::string base_name = base_it->first;
    //unsigned base_id = base_it->second;
    //if (!compPartition.at(base_id))
      if (canServicePartition(base_name))
        return 1;
  }
  return 0;
}
bool Scratchpad::canServicePartition(std::string baseName)
{
  unsigned partition_id = findPartitionID(baseName);
  if (compPartition.at(partition_id))
    return 1;
  else if (occupiedBWPerPartition.at(partition_id) < numOfPortsPerPartition)
    return 1;
  else
    return 0;
}

unsigned Scratchpad::findPartitionID(std::string baseName)
{

  auto partition_it = baseToPartitionID.find(baseName);
  if (partition_it != baseToPartitionID.end())
    return partition_it->second;
  else
  {
    std::cerr << "Unknown Partition Name:" << baseName << std::endl;
    std::cerr << "Need to Explicitly Declare How to Partition Each Array in the Config File" << std::endl;
    exit(0);
  }
}
// power in mW, energy in nJ, time in ns
void Scratchpad::getAveragePower(unsigned int cycles, float *avg_power,
                                 float *avg_dynamic, float *avg_leak)
{
  float load_energy = 0;
  float store_energy = 0;
  float leakage_power = 0;
  for (auto it = partition_loads.begin(); it != partition_loads.end(); ++it)
    load_energy += it->second * getReadEnergy(it->first);
  for (auto it = partition_stores.begin(); it != partition_stores.end(); ++it)
    store_energy += it->second * getReadEnergy(it->first);
  std::vector<std::string> all_partitions;
  getMemoryBlocks(all_partitions);
  for (auto it = all_partitions.begin(); it != all_partitions.end(); ++it)
    leakage_power += getLeakagePower(*it) ;

  // Load power and store power are computed per cycle, so we have to average
  // the aggregated per cycle power.
  *avg_dynamic = (load_energy + store_energy) * 1000 / (cycleTime *cycles) ;
  *avg_leak = leakage_power;
  *avg_power =  *avg_dynamic + *avg_leak;
}

float Scratchpad::getTotalArea() {
  std::vector<std::string> all_partitions;
  getMemoryBlocks(all_partitions);
  float mem_area = 0;
  for (auto it = all_partitions.begin(); it != all_partitions.end() ; ++it)
    mem_area += getArea(*it);
  return mem_area;
}

void Scratchpad::getMemoryBlocks(std::vector<std::string> &names)
{
  for (auto base_it = baseToPartitionID.begin();
       base_it != baseToPartitionID.end(); ++base_it)
  {
    std::string base_name = base_it->first;
    unsigned base_id = base_it->second;
    if (!compPartition.at(base_id))
      names.push_back(base_name);
  }
}

void Scratchpad::increment_loads(std::string partition)
{
  partition_loads[partition]++;
}

void Scratchpad::increment_stores(std::string partition)
{
  partition_stores[partition]++;
}

void Scratchpad::getRegisterBlocks(std::vector<std::string> &names)
{
  for(auto base_it = baseToPartitionID.begin(); base_it != baseToPartitionID.end(); ++base_it)
  {
    std::string base_name = base_it->first;
    unsigned base_id = base_it->second;
    if (compPartition.at(base_id))
      names.push_back(base_name);
  }
}

float Scratchpad::getReadEnergy(std::string baseName)
{
  unsigned partition_id = findPartitionID(baseName);
  return readEnergyPerPartition.at(partition_id);
}

float Scratchpad::getWriteEnergy(std::string baseName)
{
  unsigned partition_id = findPartitionID(baseName);
  return writeEnergyPerPartition.at(partition_id);
}

float Scratchpad::getLeakagePower(std::string baseName)
{
  unsigned partition_id = findPartitionID(baseName);
  return leakPowerPerPartition.at(partition_id);
}

float Scratchpad::getArea(std::string baseName)
{
  unsigned partition_id = findPartitionID(baseName);
  return areaPerPartition.at(partition_id);
}

uca_org_t Scratchpad::cactiWrapper(unsigned num_of_bytes, unsigned wordsize)
{
  int cache_size = num_of_bytes;
  int line_size = wordsize; // in bytes
  if (wordsize < 4 ) //minimum line size in cacti is 32-bit/4-byte
    line_size = 4;
  if (cache_size / line_size < 64)
    cache_size = line_size * 64; //minimum scratchpad size: 64 words
  int associativity = 1;
  int rw_ports = RW_PORTS;
  int excl_read_ports = 0;
  int excl_write_ports = 0;
  int single_ended_read_ports = 0;
  int search_ports = 0;
  int banks = 1;
  double tech_node = 45; // in nm
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
  int access_mode = 2; //0 normal, 1 seq, 2 fast
  int cache = 0; //scratch ram 0 or cache 1
  int main_mem = 0;
  //assign weights for CACTI optimizations
  int obj_func_delay = 0;
  int obj_func_dynamic_power = 0;
  int obj_func_leakage_power = 1000000;
  int obj_func_area = 0;
  int obj_func_cycle_time = 0;
  //from CACTI example config...
  int dev_func_delay = 100;
  int dev_func_dynamic_power = 1000;
  int dev_func_leakage_power = 10;
  int dev_func_area = 100;
  int dev_func_cycle_time = 100;

  int ed_ed2_none = 2; // 0 - ED, 1 - ED^2, 2 - use weight and deviate
  int temp = 300;
  int wt = 0; //0 - default(search across everything), 1 - global, 2 - 5% delay penalty, 3 - 10%, 4 - 20 %, 5 - 30%, 6 - low-swing
  int data_arr_ram_cell_tech_flavor_in = 0;  // 0(itrs-hp) 1-itrs-lstp(low standby power)
  int data_arr_peri_global_tech_flavor_in = 0; // 0(itrs-hp)
  int tag_arr_ram_cell_tech_flavor_in = 2; // itrs-hp
  int tag_arr_peri_global_tech_flavor_in = 2; // itrs-hp
  int interconnect_projection_type_in = 1; // 0 - aggressive, 1 - normal
  int wire_inside_mat_type_in = 2; // 2 - global, 0 - local, 1 - semi-global
  int wire_outside_mat_type_in = 2; // 2 - global
  int REPEATERS_IN_HTREE_SEGMENTS_in = 1;//TODO for now only wires with repeaters are supported
  int p_input = 0; //print input parameters
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
  return cacti_interface(
      cache_size,
      line_size,
      associativity,
      rw_ports,
      excl_read_ports,
      excl_write_ports,
      single_ended_read_ports,
      search_ports,
      banks,
      tech_node, // in nm
      output_width,
      specific_tag,
      tag_width,
      access_mode, //0 normal, 1 seq, 2 fast
      cache, //scratch ram or cache
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
      wiretype ,
      force_config,
      ndwl,
      ndbl,
      nspd,
      ndcm,
      ndsam1,
      ndsam2,
      ecc);
}
