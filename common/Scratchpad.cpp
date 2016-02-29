#include "generic_func.h"
#include "Scratchpad.h"

Scratchpad::Scratchpad(unsigned p_ports_per_part, float cycle_time) {
  numOfPortsPerPartition = p_ports_per_part;
  cycleTime = cycle_time;
}

Scratchpad::~Scratchpad() {}

void Scratchpad::clear() {
  numOfPartitions = 0;
  partitions.clear();
  cacti_value.clear();
}
// wordsize in bytes
void Scratchpad::setScratchpad(std::string baseName,
                               unsigned num_of_bytes,
                               unsigned wordsize) {
  assert(!partitionExist(baseName));
  // size: number of words

  partition_t curr_part;
  curr_part.size = num_of_bytes;

  // set read/write/leak/area per partition
  cacti_key _cacti_key = std::make_pair(num_of_bytes, wordsize);
  uca_org_t cacti_result;
  auto cacti_it = cacti_value.find(_cacti_key);
  if (cacti_it == cacti_value.end()) {
    cacti_result = cactiWrapper(num_of_bytes, wordsize);
    cacti_value[_cacti_key] = cacti_result;
  } else {
    cacti_result = cacti_it->second;
  }
  // power in mW, energy in pJ, area in mm2
  curr_part.read_energy = cacti_result.power.readOp.dynamic * 1e+12;
  curr_part.write_energy = cacti_result.power.writeOp.dynamic * 1e+12;
  curr_part.leak_power = cacti_result.power.readOp.leakage * 1000;
  curr_part.area = cacti_result.area;
  partitions[baseName] = curr_part;
}

void Scratchpad::step() {
  for (auto it = partitions.begin(); it != partitions.end(); ++it)
    it->second.occupied_bw = 0;
}
bool Scratchpad::partitionExist(std::string baseName) {
  auto partition_it = partitions.find(baseName);
  if (partition_it != partitions.end())
    return true;
  else
    return false;
}
bool Scratchpad::addressRequest(std::string baseName) {
  if (canServicePartition(baseName)) {
    partitions[baseName].occupied_bw++;
    return true;
  } else
    return false;
}
bool Scratchpad::canService() {
  for (auto it = partitions.begin(); it != partitions.end(); ++it) {
    if (it->second.occupied_bw < numOfPortsPerPartition)
      return true;
  }
  return false;
}
bool Scratchpad::canServicePartition(std::string baseName) {
  if (partitions[baseName].occupied_bw < numOfPortsPerPartition)
    return true;
  else
    return false;
}

// power in mW, energy in pJ, time in ns
void Scratchpad::getAveragePower(unsigned int cycles,
                                 float* avg_power,
                                 float* avg_dynamic,
                                 float* avg_leak) {
  float load_energy = 0;
  float store_energy = 0;
  float leakage_power = 0;
  for (auto it = partitions.begin(); it != partitions.end(); ++it) {
    load_energy += it->second.loads * it->second.read_energy;
    store_energy += it->second.stores * it->second.write_energy;
    leakage_power += it->second.leak_power;
  }

  // Load power and store power are computed per cycle, so we have to average
  // the aggregated per cycle power.
  *avg_dynamic = (load_energy + store_energy) / (cycleTime * cycles);
  *avg_leak = leakage_power;
  *avg_power = *avg_dynamic + *avg_leak;
}

float Scratchpad::getTotalArea() {
  float mem_area = 0;
  for (auto it = partitions.begin(); it != partitions.end(); ++it)
    mem_area += it->second.area;
  return mem_area;
}

void Scratchpad::getMemoryBlocks(std::vector<std::string>& names) {
  for (auto it = partitions.begin(); it != partitions.end(); ++it) {
    names.push_back(it->first);
  }
}
void Scratchpad::increment_loads(std::string partition) {
  partitions[partition].occupied_bw++;
  partitions[partition].loads++;
}

void Scratchpad::increment_stores(std::string partition) {
  partitions[partition].occupied_bw++;
  partitions[partition].stores++;
}

void Scratchpad::increment_dma_loads(std::string partition,
                                     unsigned num_accesses) {
  partitions[partition].stores += num_accesses;
}

void Scratchpad::increment_dma_stores(std::string partition,
                                      unsigned num_accesses) {
  partitions[partition].loads += num_accesses;
}

float Scratchpad::getReadEnergy(std::string baseName) {
  return partitions[baseName].read_energy;
}

float Scratchpad::getWriteEnergy(std::string baseName) {
  return partitions[baseName].write_energy;
}

float Scratchpad::getLeakagePower(std::string baseName) {
  return partitions[baseName].leak_power;
}

float Scratchpad::getArea(std::string baseName) {
  return partitions[baseName].area;
}

uca_org_t Scratchpad::cactiWrapper(unsigned num_of_bytes, unsigned wordsize) {
  int cache_size = num_of_bytes;
  int line_size = wordsize;  // in bytes
  if (wordsize < 4)          // minimum line size in cacti is 32-bit/4-byte
    line_size = 4;
  if (cache_size / line_size < 64)
    cache_size = line_size * 64;  // minimum scratchpad size: 64 words
  int associativity = 1;
  int rw_ports = SINGLE_PORT_SPAD;
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
