#include <assert.h>
#include <sstream>
#include <utility>

#include "opcode_func.h"
#include "BaseDatapath.h"
#include "ExecNode.h"
#include "DatabaseDeps.h"
#include "graph_opts/all_graph_opts.h"

using namespace SrcTypes;

BaseDatapath::BaseDatapath(std::string& bench,
                           std::string& trace_file_name,
                           std::string& config_file)
    : benchName(bench), current_trace_off(0) {
  parse_config(benchName, config_file);

  use_db = false;
  if (!fileExists(trace_file_name)) {
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << " ERROR: Input Trace Not Found  " << std::endl;
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << "       Aladdin Ends..          " << std::endl;
    std::cerr << "-------------------------------" << std::endl;
    exit(1);
  }
  std::string file_name = benchName + "_summary";
  /* Remove the old file. */
  if (access(file_name.c_str(), F_OK) != -1 && remove(file_name.c_str()) != 0)
    perror("Failed to delete the old summary file");

  struct stat st;
  stat(trace_file_name.c_str(), &st);
  trace_size = st.st_size;
  trace_file = gzopen(trace_file_name.c_str(), "r");
}

BaseDatapath::~BaseDatapath() {
  gzclose(trace_file);
}

bool BaseDatapath::buildDddg() {
  DDDG* dddg;
  dddg = new DDDG(this, &program, trace_file);
  /* Build initial DDDG. */
  current_trace_off = dddg->build_initial_dddg(current_trace_off, trace_size);
  updateUnrollingPipeliningWithLabelInfo();
  program.loop_info.updateSamplingWithLabelInfo();
  user_params.checkOverlappingRanges();
  topLevelFunctionName = dddg->get_top_level_function_name();
  delete dddg;

  if (current_trace_off == DDDG::END_OF_TRACE)
    return false;

  std::cout << "-------------------------------" << std::endl;
  std::cout << "    Initializing BaseDatapath      " << std::endl;
  std::cout << "-------------------------------" << std::endl;
  std::cout << " Top level: " << topLevelFunctionName << std::endl;
  numTotalNodes = program.nodes.size();
  beginNodeId = program.nodes.begin()->first;
  endNodeId = (--program.nodes.end())->first + 1;

  program.createVertexMap();

  num_cycles = 0;
  upsampled = false;
  return true;
}

void BaseDatapath::resetTrace() { gzseek(trace_file, 0, SEEK_SET); }

void BaseDatapath::updateUnrollingPipeliningWithLabelInfo() {
  // The config file is parsed before the trace, so we don't have line number
  // information yet. After parsing the trace, update the unrolling and
  // pipelining config maps with line numbers.
  for (auto it = program.labelmap.begin(); it != program.labelmap.end(); ++it) {
    const UniqueLabel& label_with_num = it->second;
    UniqueLabel label_without_num(
        label_with_num.get_function(), label_with_num.get_label(), 0);

    auto unroll_it = user_params.unrolling.find(label_without_num);
    if (unroll_it != user_params.unrolling.end()) {
      unsigned factor = unroll_it->second;
      user_params.unrolling.erase(unroll_it);
      user_params.unrolling[label_with_num] = factor;
    }

    auto pipeline_it = user_params.pipeline.find(label_without_num);
    if (pipeline_it != user_params.pipeline.end()) {
      user_params.pipeline.erase(pipeline_it);
      user_params.pipeline.insert(label_with_num);
    }
  }

  // Update the unrolling/pipelining configurations for inlined labels.
  for (auto it = program.inline_labelmap.begin();
       it != program.inline_labelmap.end(); ++it) {
    const UniqueLabel& inlined_label = it->first;
    const UniqueLabel& orig_label = it->second;

    auto unroll_it = user_params.unrolling.find(orig_label);
    if (unroll_it != user_params.unrolling.end())
      user_params.unrolling[inlined_label] = unroll_it->second;

    auto pipeline_it = user_params.pipeline.find(orig_label);
    if (pipeline_it != user_params.pipeline.end())
      user_params.pipeline.insert(inlined_label);
  }
}

void BaseDatapath::clearDatapath() {
  program.clear();
  clearFunctionName();
  clearArrayBaseAddress();
  clearRegStats();
}

void BaseDatapath::initBaseAddress() {
  auto opt = getGraphOpt<BaseAddressInit>();
  opt->run();
#if 0
  // TODO: writing the base addresses can cause simulation to freeze when no
  // partitioning is applied to arrays due to how writeBaseAddress() parses the
  // partition number from an array's label. So this is going to be disabled
  // for the time being, until we find a chance to fix this.
  writeBaseAddress();
#endif
}

void BaseDatapath::initDmaBaseAddress() {
  auto opt = getGraphOpt<DmaBaseAddressInit>();
  opt->run();
}

void BaseDatapath::memoryAmbiguation() {
  auto opt = getGraphOpt<MemoryAmbiguationOpt>();
  opt->run();
}

void BaseDatapath::removePhiNodes() {
  auto opt = getGraphOpt<PhiNodeRemoval>();
  opt->run();
}

void BaseDatapath::loopFlatten() {
  auto opt = getGraphOpt<LoopFlattening>();
  opt->run();
}

void BaseDatapath::removeInductionDependence() {
  auto opt = getGraphOpt<InductionDependenceRemoval>();
  opt->run();
}

void BaseDatapath::loopPipelining() {
  auto opt = getGraphOpt<GlobalLoopPipelining>();
  opt->run();
}

void BaseDatapath::perLoopPipelining() {
  auto opt = getGraphOpt<PerLoopPipelining>();
  opt->run();
}

void BaseDatapath::loopUnrolling() {
  auto opt = getGraphOpt<LoopUnrolling>();
  opt->run();
  // Loop unrolling pass has identified and stored all loop boundaries in a flat
  // list. Based on that, now we build a tree to represent the hierarchical loop
  // structure. This tree will be used later by loop pipelining and loop
  // sampling.
  program.loop_info.buildLoopTree();
}

void BaseDatapath::fuseRegLoadStores() {
  auto opt = getGraphOpt<RegLoadStoreFusion>();
  opt->run();
}

void BaseDatapath::fuseConsecutiveBranches() {
  auto opt = getGraphOpt<ConsecutiveBranchFusion>();
  opt->run();
}

void BaseDatapath::removeSharedLoads() {
  auto opt = getGraphOpt<LoadBuffering>();
  opt->run();
}

void BaseDatapath::storeBuffer() {
  auto opt = getGraphOpt<StoreBuffering>();
  opt->run();
}

void BaseDatapath::removeRepeatedStores() {
  auto opt = getGraphOpt<RepeatedStoreRemoval>();
  opt->run();
}

void BaseDatapath::treeHeightReduction() {
  auto opt = getGraphOpt<TreeHeightReduction>();
  opt->run();
}

// called in the end of the whole flow
void BaseDatapath::dumpStats() {
  rescheduleNodesWhenNeeded();
  upsampleLoops();
  computeRegStats();
  writePerCycleActivity();
  writeOtherStats();
#ifdef DEBUG
  dumpGraph(benchName);
#endif
}

void BaseDatapath::upsampleLoops() {
  // Update num_cycles with the correction cycles.
  upsampled_cycles = program.loop_info.upsampleLoops();
  num_cycles += upsampled_cycles;
  upsampled = true;
}

/*
 * Write per cycle activity to bench_stats. The format is:
 * cycle_num,num-of-muls,num-of-adds,num-of-bitwise-ops,num-of-reg-reads,num-of-reg-writes
 * If it is called from ScratchpadDatapath, it also outputs per cycle memory
 * activity for each partitioned array.
 */
void BaseDatapath::writePerCycleActivity() {
  /* Activity per function in the code. Indexed by function names.  */
  cycle_activity_map<FunctionActivity> func_activity;
  std::unordered_map<std::string, FunctionActivity> func_max_activity;
  /* Activity per array. Indexed by array names.  */
  cycle_activity_map<MemoryActivity> mem_activity;

  std::vector<std::string> comp_partition_names;
  std::vector<std::string> mem_partition_names;
  registers.getRegisterNames(comp_partition_names);
  getMemoryBlocks(mem_partition_names);

  initPerCycleActivity(comp_partition_names,
                       mem_partition_names,
                       mem_activity,
                       func_activity,
                       func_max_activity,
                       num_cycles);

  updatePerCycleActivity(mem_activity, func_activity, func_max_activity);

  outputPerCycleActivity(comp_partition_names,
                         mem_partition_names,
                         mem_activity,
                         func_activity,
                         func_max_activity);
}

void BaseDatapath::initPerCycleActivity(
    std::vector<std::string>& comp_partition_names,
    std::vector<std::string>& mem_partition_names,
    cycle_activity_map<MemoryActivity>& mem_activity,
    cycle_activity_map<FunctionActivity>& func_activity,
    std::unordered_map<std::string, FunctionActivity>& func_max_activity,
    int num_cycles) {
  for (auto it = comp_partition_names.begin(); it != comp_partition_names.end();
       ++it) {
    mem_activity.insert(
        { *it, std::vector<MemoryActivity>(num_cycles, { 0, 0 }) });
  }
  for (auto it = mem_partition_names.begin(); it != mem_partition_names.end();
       ++it) {
    mem_activity.insert(
        { *it, std::vector<MemoryActivity>(num_cycles, { 0, 0 }) });
  }
  for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
    FunctionActivity tmp;
    func_activity.insert({ *it, std::vector<FunctionActivity>(num_cycles, tmp) });
    func_max_activity.insert({ *it, tmp });
  }
}

void BaseDatapath::updatePerCycleActivity(
    cycle_activity_map<MemoryActivity>& mem_activity,
    cycle_activity_map<FunctionActivity>& func_activity,
    std::unordered_map<std::string, FunctionActivity>& func_max_activity) {
  /* We use two ways to count the number of functional units in accelerators:
   * one assumes that functional units can be reused in the same region; the
   * other assumes no reuse of functional units. The advantage of reusing is
   * that it eliminates the cost of duplicating functional units which can lead
   * to high leakage power and area. However, additional wires and muxes may
   * need to be added for reusing.
   * In the current model, we assume that multipliers can be reused, since the
   * leakage power and area of multipliers are relatively significant, and no
   * reuse for adders. This way of modeling is consistent with our observation
   * of accelerators generated with Vivado. */
  unsigned num_adds_so_far = 0, num_bits_so_far = 0, num_shifters_so_far = 0;
  auto bound_it = program.loop_bounds.begin();
  for (auto node_it = program.nodes.begin(); node_it != program.nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    // TODO: On every iteration, this could be a bottleneck.
    const std::string& func_id = node->get_static_function()->get_name();
    auto max_it = func_max_activity.find(func_id);
    assert(max_it != func_max_activity.end());

    if (bound_it != program.loop_bounds.end() &&
        node->get_node_id() == bound_it->node_id) {
      if (max_it->second.add < num_adds_so_far)
        max_it->second.add = num_adds_so_far;
      if (max_it->second.bit < num_bits_so_far)
        max_it->second.bit = num_bits_so_far;
      if (max_it->second.shifter < num_shifters_so_far)
        max_it->second.shifter = num_shifters_so_far;
      num_adds_so_far = 0;
      num_bits_so_far = 0;
      num_shifters_so_far = 0;
      bound_it++;
    }
    if (node->is_isolated())
      continue;
    int node_level = node->get_start_execution_cycle();
    FunctionActivity& curr_fu_activity = func_activity.at(func_id).at(node_level);

    if (node->is_multicycle_op()) {
      for (int stage = 0;
           node_level + stage < node->get_complete_execution_cycle();
           stage++) {
        FunctionActivity& fp_fu_activity =
            func_activity.at(func_id).at(node_level + stage);
        /* Activity for floating point functional units includes all their
         * stages.*/
        if (node->is_fp_add_op()) {
          if (node->is_double_precision())
            fp_fu_activity.fp_dp_add += 1;
          else
            fp_fu_activity.fp_sp_add += 1;
        } else if (node->is_fp_mul_op()) {
          if (node->is_double_precision())
            fp_fu_activity.fp_dp_mul += 1;
          else
            fp_fu_activity.fp_sp_mul += 1;
        } else if (node->is_special_math_op()) {
          fp_fu_activity.trig += 1;
        }
      }
    } else if (node->is_int_mul_op()) {
      curr_fu_activity.mul += 1;
    } else if (node->is_int_add_op()) {
      curr_fu_activity.add += 1;
      num_adds_so_far += 1;
    } else if (node->is_shifter_op()) {
      curr_fu_activity.shifter += 1;
      num_shifters_so_far += 1;
    } else if (node->is_bit_op()) {
      curr_fu_activity.bit += 1;
      num_bits_so_far += 1;
    } else if (node->is_load_op()) {
      const std::string& array_label = node->get_array_label();
      int bytes_read = node->get_mem_access()->size;
      if (mem_activity.find(array_label) != mem_activity.end()) {
        mem_activity.at(array_label).at(node_level).bytes_read += bytes_read;
      }
    } else if (node->is_store_op()) {
      const std::string& array_label = node->get_array_label();
      int bytes_written = node->get_mem_access()->size;
      if (mem_activity.find(array_label) != mem_activity.end())
        mem_activity.at(array_label).at(node_level).bytes_write +=
            bytes_written;
    }
  }
  for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
    auto max_it = func_max_activity.find(*it);
    assert(max_it != func_max_activity.end());
    std::vector<FunctionActivity>& cycle_activity = func_activity.at(*it);
    max_it->second.mul =
        std::max_element(cycle_activity.begin(),
                         cycle_activity.end(),
                         [](const FunctionActivity& a, const FunctionActivity& b) {
                           return (a.mul < b.mul);
                         })->mul;
    max_it->second.fp_sp_mul =
        std::max_element(cycle_activity.begin(),
                         cycle_activity.end(),
                         [](const FunctionActivity& a, const FunctionActivity& b) {
                           return (a.fp_sp_mul < b.fp_sp_mul);
                         })->fp_sp_mul;
    max_it->second.fp_dp_mul =
        std::max_element(cycle_activity.begin(),
                         cycle_activity.end(),
                         [](const FunctionActivity& a, const FunctionActivity& b) {
                           return (a.fp_dp_mul < b.fp_dp_mul);
                         })->fp_dp_mul;
    max_it->second.fp_sp_add =
        std::max_element(cycle_activity.begin(),
                         cycle_activity.end(),
                         [](const FunctionActivity& a, const FunctionActivity& b) {
                           return (a.fp_sp_add < b.fp_sp_add);
                         })->fp_sp_add;
    max_it->second.fp_dp_add =
        std::max_element(cycle_activity.begin(),
                         cycle_activity.end(),
                         [](const FunctionActivity& a, const FunctionActivity& b) {
                           return (a.fp_dp_add < b.fp_dp_add);
                         })->fp_dp_add;
    max_it->second.trig =
        std::max_element(cycle_activity.begin(),
                         cycle_activity.end(),
                         [](const FunctionActivity& a, const FunctionActivity& b) {
                           return (a.trig < b.trig);
                         })->trig;
  }
}

void BaseDatapath::outputPerCycleActivity(
    std::vector<std::string>& comp_partition_names,
    std::vector<std::string>& mem_partition_names,
    cycle_activity_map<MemoryActivity>& mem_activity,
    cycle_activity_map<FunctionActivity>& func_activity,
    std::unordered_map<std::string, FunctionActivity>& func_max_activity) {
  /*Set the constants*/
  float add_int_power, add_switch_power, add_leak_power, add_area;
  float mul_int_power, mul_switch_power, mul_leak_power, mul_area;
  float fp_sp_mul_int_power, fp_sp_mul_switch_power, fp_sp_mul_leak_power,
      fp_sp_mul_area;
  float fp_dp_mul_int_power, fp_dp_mul_switch_power, fp_dp_mul_leak_power,
      fp_dp_mul_area;
  float fp_sp_add_int_power, fp_sp_add_switch_power, fp_sp_add_leak_power,
      fp_sp_add_area;
  float fp_dp_add_int_power, fp_dp_add_switch_power, fp_dp_add_leak_power,
      fp_dp_add_area;
  float trig_int_power, trig_switch_power, trig_leak_power, trig_area;
  float reg_int_power_per_bit, reg_switch_power_per_bit;
  float reg_leak_power_per_bit, reg_area_per_bit;
  float bit_int_power, bit_switch_power, bit_leak_power, bit_area;
  float shifter_int_power, shifter_switch_power, shifter_leak_power,
      shifter_area;
  unsigned idle_fu_cycles = 0;

  float cycleTime = user_params.cycle_time;
  getAdderPowerArea(
      cycleTime, &add_int_power, &add_switch_power, &add_leak_power, &add_area);
  getMultiplierPowerArea(
      cycleTime, &mul_int_power, &mul_switch_power, &mul_leak_power, &mul_area);
  getRegisterPowerArea(cycleTime,
                       &reg_int_power_per_bit,
                       &reg_switch_power_per_bit,
                       &reg_leak_power_per_bit,
                       &reg_area_per_bit);
  getBitPowerArea(
      cycleTime, &bit_int_power, &bit_switch_power, &bit_leak_power, &bit_area);
  getShifterPowerArea(cycleTime,
                      &shifter_int_power,
                      &shifter_switch_power,
                      &shifter_leak_power,
                      &shifter_area);
  getSinglePrecisionFloatingPointMultiplierPowerArea(cycleTime,
                                                     &fp_sp_mul_int_power,
                                                     &fp_sp_mul_switch_power,
                                                     &fp_sp_mul_leak_power,
                                                     &fp_sp_mul_area);
  getDoublePrecisionFloatingPointMultiplierPowerArea(cycleTime,
                                                     &fp_dp_mul_int_power,
                                                     &fp_dp_mul_switch_power,
                                                     &fp_dp_mul_leak_power,
                                                     &fp_dp_mul_area);
  getSinglePrecisionFloatingPointAdderPowerArea(cycleTime,
                                                &fp_sp_add_int_power,
                                                &fp_sp_add_switch_power,
                                                &fp_sp_add_leak_power,
                                                &fp_sp_add_area);
  getDoublePrecisionFloatingPointAdderPowerArea(cycleTime,
                                                &fp_dp_add_int_power,
                                                &fp_dp_add_switch_power,
                                                &fp_dp_add_leak_power,
                                                &fp_dp_add_area);
  getTrigonometricFunctionPowerArea(cycleTime,
                                    &trig_int_power,
                                    &trig_switch_power,
                                    &trig_leak_power,
                                    &trig_area);

  std::string file_name;
#ifdef DEBUG
  // TODO: Wrap the per-cycle stats output in a class dedicated to writing
  // cycle-level stats.
  file_name = benchName + "_stats.txt";
  std::ofstream stats;
  stats.open(file_name.c_str(), std::ofstream::out | std::ofstream::app);
  stats << "cycles," << num_cycles << "," << numTotalNodes << std::endl;
  stats << num_cycles << ",";

  std::ofstream power_stats;
  file_name = benchName + "_power_stats.txt";
  power_stats.open(file_name.c_str(), std::ofstream::out | std::ofstream::app);
  power_stats << "cycles," << num_cycles << "," << numTotalNodes << std::endl;
  power_stats << num_cycles << ",";

  /*Start writing the second line*/
  for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
    stats << *it << "-fp-sp-mul," << *it << "-fp-dp-mul," << *it
          << "-fp-sp-add," << *it << "-fp-dp-add," << *it << "-mul," << *it
          << "-add," << *it << "-bit," << *it << "-shifter," << *it << "-trig,";
    power_stats << *it << "-fp-sp-mul," << *it << "-fp-dp-mul," << *it
                << "-fp-sp-add," << *it << "-fp-dp-add," << *it << "-mul,"
                << *it << "-add," << *it << "-bit," << *it << "-shifter," << *it
                << "-trig,";
  }
  // TODO: mem_partition_names contains logical arrays, not completely
  // partitioned arrays.
  stats << "reg-read,reg-write,";
  for (auto it = mem_partition_names.begin(); it != mem_partition_names.end();
       ++it) {
    stats << *it << "-read," << *it << "-write,";
  }
  stats << std::endl;
  power_stats << "reg-read,reg-write,";
  power_stats << std::endl;
#endif
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
  int max_fp_sp_mul = 0, max_fp_dp_mul = 0;
  int max_fp_sp_add = 0, max_fp_dp_add = 0;
  int max_trig = 0;
  for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
    auto max_it = func_max_activity.find(*it);
    assert(max_it != func_max_activity.end());
    max_bit += max_it->second.bit;
    max_add += max_it->second.add;
    max_mul += max_it->second.mul;
    max_shifter += max_it->second.shifter;
    max_fp_sp_mul += max_it->second.fp_sp_mul;
    max_fp_dp_mul += max_it->second.fp_dp_mul;
    max_fp_sp_add += max_it->second.fp_sp_add;
    max_fp_dp_add += max_it->second.fp_dp_add;
    max_trig += max_it->second.trig;
  }

  float add_leakage_power = add_leak_power * max_add;
  float mul_leakage_power = mul_leak_power * max_mul;
  float bit_leakage_power = bit_leak_power * max_bit;
  float shifter_leakage_power = shifter_leak_power * max_shifter;
  // Total leakage power is the sum of leakage from the explicitly named
  // registers (completely partitioned registers) and the inferred flops from
  // the functional units.
  float fu_flop_leakage_power = reg_leak_power_per_bit * 32 * max_reg;
  float reg_leakage_power =
      registers.getTotalLeakagePower() + fu_flop_leakage_power;
  float fp_sp_mul_leakage_power = fp_sp_mul_leak_power * max_fp_sp_mul;
  float fp_dp_mul_leakage_power = fp_dp_mul_leak_power * max_fp_dp_mul;
  float fp_sp_add_leakage_power = fp_sp_add_leak_power * max_fp_sp_add;
  float fp_dp_add_leakage_power = fp_dp_add_leak_power * max_fp_dp_add;
  float trig_leakage_power = trig_leak_power * max_trig;
  float fu_leakage_power = mul_leakage_power + add_leakage_power +
                           reg_leakage_power + bit_leakage_power +
                           shifter_leakage_power + fp_sp_mul_leakage_power +
                           fp_dp_mul_leakage_power + fp_sp_add_leakage_power +
                           fp_dp_add_leakage_power + trig_leakage_power;
  /*Finish calculating the number of FUs and leakage power*/

  float fu_dynamic_energy = 0;

  /*Start writing per cycle activity */
  for (unsigned curr_level = 0; ((int)curr_level) < num_cycles; ++curr_level) {
#ifdef DEBUG
    stats << curr_level << ",";
    power_stats << curr_level << ",";
#endif
    bool is_fu_idle = true;
    // For FUs
    for (auto it = functionNames.begin(); it != functionNames.end(); ++it) {
      FunctionActivity& curr_activity = func_activity.at(*it).at(curr_level);
      is_fu_idle &= curr_activity.is_idle();
#ifdef DEBUG
      stats << curr_activity.fp_sp_mul << ","
            << curr_activity.fp_dp_mul << ","
            << curr_activity.fp_sp_add << ","
            << curr_activity.fp_dp_add << ","
            << curr_activity.mul << ","
            << curr_activity.add << ","
            << curr_activity.bit << ","
            << curr_activity.shifter << ","
            << curr_activity.trig << ",";
#endif
      float curr_fp_sp_mul_dynamic_power =
          (fp_sp_mul_switch_power + fp_sp_mul_int_power) *
          curr_activity.fp_sp_mul;
      float curr_fp_dp_mul_dynamic_power =
          (fp_dp_mul_switch_power + fp_dp_mul_int_power) *
          curr_activity.fp_dp_mul;
      float curr_fp_sp_add_dynamic_power =
          (fp_sp_add_switch_power + fp_sp_add_int_power) *
          curr_activity.fp_sp_add;
      float curr_fp_dp_add_dynamic_power =
          (fp_dp_add_switch_power + fp_dp_add_int_power) *
          curr_activity.fp_dp_add;
      float curr_trig_dynamic_power =
          (trig_switch_power + trig_int_power) *
          curr_activity.trig;
      float curr_mul_dynamic_power = (mul_switch_power + mul_int_power) *
                                     curr_activity.mul;
      float curr_add_dynamic_power = (add_switch_power + add_int_power) *
                                     curr_activity.add;
      float curr_bit_dynamic_power = (bit_switch_power + bit_int_power) *
                                     curr_activity.bit;
      float curr_shifter_dynamic_power =
          (shifter_switch_power + shifter_int_power) *
          curr_activity.shifter;
      fu_dynamic_energy +=
          (curr_fp_sp_mul_dynamic_power + curr_fp_dp_mul_dynamic_power +
           curr_fp_sp_add_dynamic_power + curr_fp_dp_add_dynamic_power +
           curr_trig_dynamic_power + curr_mul_dynamic_power +
           curr_add_dynamic_power + curr_bit_dynamic_power +
           curr_shifter_dynamic_power) *
          cycleTime;
#ifdef DEBUG
      power_stats << curr_fp_sp_mul_dynamic_power << ","
                  << curr_fp_dp_mul_dynamic_power << ","
                  << curr_fp_sp_add_dynamic_power << ","
                  << curr_fp_dp_add_dynamic_power << ","
                  << curr_mul_dynamic_power << ","
                  << curr_add_dynamic_power << ","
                  << curr_bit_dynamic_power << ","
                  << curr_shifter_dynamic_power << ","
                  << curr_trig_dynamic_power << ",";
#endif
    }
    // These are inferred flops from functional unit activity. They are assumed
    // to be 4-byte accesses.
    int curr_reg_reads = regStats.at(curr_level).reads;
    int curr_reg_writes = regStats.at(curr_level).writes;
    float curr_reg_read_dynamic_energy =
        (reg_int_power_per_bit + reg_switch_power_per_bit) * curr_reg_reads *
        32 * cycleTime;
    float curr_reg_write_dynamic_energy =
        (reg_int_power_per_bit + reg_switch_power_per_bit) * curr_reg_writes *
        32 * cycleTime;
    for (auto it = comp_partition_names.begin();
         it != comp_partition_names.end();
         ++it) {
      Register* reg = registers.getRegister(*it);
      int reg_bytes_read = mem_activity.at(*it).at(curr_level).bytes_read;
      int reg_bytes_written = mem_activity.at(*it).at(curr_level).bytes_write;
      int reg_words_read = ceil(((float)reg_bytes_read)/reg->getWordSize());
      int reg_words_written = ceil(((float)reg_bytes_written)/reg->getWordSize());
      int reg_read_energy = registers.getReadEnergy(*it) * reg_words_read;
      int reg_write_energy = registers.getReadEnergy(*it) * reg_words_written;

      curr_reg_reads += reg_words_read;
      curr_reg_writes += reg_words_written;
      curr_reg_read_dynamic_energy += reg_read_energy;
      curr_reg_write_dynamic_energy += reg_write_energy;
    }
    fu_dynamic_energy +=
        curr_reg_read_dynamic_energy + curr_reg_write_dynamic_energy;

#ifdef DEBUG
    stats << curr_reg_reads << "," << curr_reg_writes << ",";

    for (auto it = mem_partition_names.begin(); it != mem_partition_names.end();
         ++it) {
      const std::string& array_name = *it;
      const PartitionEntry &entry = user_params.getArrayConfig(array_name);
      int mem_bytes_read = mem_activity.at(array_name).at(curr_level).bytes_read;
      int mem_bytes_written = mem_activity.at(array_name).at(curr_level).bytes_write;
      int mem_words_read = ceil(((float)mem_bytes_read) / entry.wordsize);
      int mem_words_written = ceil(((float)mem_bytes_written) / entry.wordsize);
      stats << mem_words_read << "," << mem_words_written << ",";
    }
    stats << std::endl;
    power_stats << (curr_reg_read_dynamic_energy / cycleTime) +
                       (reg_leakage_power / 2)
                << ","
                << (curr_reg_write_dynamic_energy / cycleTime) +
                       (reg_leakage_power / 2)
                << ",";
    power_stats << std::endl;
#endif
    if (is_fu_idle)
      idle_fu_cycles++;
  }
#ifdef DEBUG
  stats.close();
  power_stats.close();
#endif

  float avg_mem_power = 0, avg_mem_dynamic_power = 0, mem_leakage_power = 0;

  getAverageMemPower(
      num_cycles, &avg_mem_power, &avg_mem_dynamic_power, &mem_leakage_power);

  float avg_fu_dynamic_power = fu_dynamic_energy / (cycleTime * num_cycles);
  float avg_fu_power = avg_fu_dynamic_power + fu_leakage_power;
  float avg_power = avg_fu_power + avg_mem_power;

  float mem_area = getTotalMemArea();
  float fu_area =
      registers.getTotalArea() + add_area * max_add + mul_area * max_mul +
      reg_area_per_bit * 32 * max_reg + bit_area * max_bit +
      shifter_area * max_shifter + fp_sp_mul_area * max_fp_sp_mul +
      fp_dp_mul_area * max_fp_dp_mul + fp_sp_add_area * max_fp_sp_add +
      fp_dp_add_area * max_fp_dp_add + trig_area * max_trig;
  float total_area = mem_area + fu_area;

  // Summary output.
  summary_data_t summary;
  summary.benchName = benchName;
  summary.topLevelFunctionName = topLevelFunctionName;
  summary.num_cycles = num_cycles;
  summary.upsampled_cycles = upsampled_cycles;
  summary.idle_fu_cycles = idle_fu_cycles;
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
  summary.max_reg = max_reg;
  summary.max_fp_sp_mul = max_fp_sp_mul;
  summary.max_fp_dp_mul = max_fp_dp_mul;
  summary.max_fp_sp_add = max_fp_sp_add;
  summary.max_fp_dp_add = max_fp_dp_add;
  summary.max_trig = max_trig;

  writeSummary(std::cout, summary);
  std::ofstream summary_file;
  file_name = benchName + "_summary";
  summary_file.open(file_name.c_str(), std::ofstream::out | std::ofstream::app);
  writeSummary(summary_file, summary);
  summary_file.close();

#ifdef USE_DB
  if (use_db)
    writeSummaryToDatabase(summary);
#endif
}

void BaseDatapath::writeSummary(std::ostream& outfile,
                                summary_data_t& summary) {
  outfile << "===============================" << std::endl;
  outfile << "        Aladdin Results        " << std::endl;
  outfile << "===============================" << std::endl;
  outfile << "Running : " << summary.benchName << std::endl;
  outfile << "Top level function: " << summary.topLevelFunctionName << std::endl;
  outfile << "Cycle : " << summary.num_cycles << " cycles" << std::endl;
  outfile << "Upsampled Cycle : " << summary.upsampled_cycles << " cycles"
          << std::endl;
  outfile << "Avg Power: " << summary.avg_power << " mW" << std::endl;
  outfile << "Idle FU Cycles: " << summary.idle_fu_cycles << " cycles" << std::endl;
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
  if (summary.max_fp_sp_mul != 0)
    outfile << "Num of Single Precision FP Multipliers: "
            << summary.max_fp_sp_mul << std::endl;
  if (summary.max_fp_sp_add != 0)
    outfile << "Num of Single Precision FP Adders: " << summary.max_fp_sp_add
            << std::endl;
  if (summary.max_fp_dp_mul != 0)
    outfile << "Num of Double Precision FP Multipliers: "
            << summary.max_fp_dp_mul << std::endl;
  if (summary.max_fp_dp_add != 0)
    outfile << "Num of Double Precision FP Adders: " << summary.max_fp_dp_add
            << std::endl;
  if (summary.max_trig != 0)
    outfile << "Num of Trigonometric Units: " << summary.max_trig << std::endl;
  if (summary.max_mul != 0)
    outfile << "Num of Multipliers (32-bit): " << summary.max_mul << std::endl;
  if (summary.max_add != 0)
    outfile << "Num of Adders (32-bit): " << summary.max_add << std::endl;
  if (summary.max_bit != 0)
    outfile << "Num of Bit-wise Operators (32-bit): " << summary.max_bit
            << std::endl;
  if (summary.max_shifter != 0)
    outfile << "Num of Shifters (32-bit): " << summary.max_shifter << std::endl;
  outfile << "Num of Registers (32-bit): " << summary.max_reg << std::endl;
  outfile << "===============================" << std::endl;
  outfile << "        Aladdin Results        " << std::endl;
  outfile << "===============================" << std::endl;
}

void BaseDatapath::writeBaseAddress() {
  std::ostringstream file_name;
  file_name << benchName << "_baseAddr.gz";
  gzFile gzip_file;
  gzip_file = gzopen(file_name.str().c_str(), "w");
  for (auto it = program.nodes.begin(), E = program.nodes.end(); it != E; ++it) {
    char original_label[256];
    int partition_id;
    const std::string& partitioned_label = it->second->get_array_label();
    if (partitioned_label.empty())
      continue;
    int num_fields = sscanf(
        partitioned_label.c_str(), "%[^-]-%d", original_label, &partition_id);
    if (num_fields != 2)
      continue;
    gzprintf(gzip_file,
             "node:%u,part:%s,base:%lld\n",
             it->first,
             partitioned_label.c_str(),
             getBaseAddress(original_label));
  }
  gzclose(gzip_file);
}

void BaseDatapath::writeOtherStats() {}

// stepFunctions
// multiple function, each function is a separate graph
void BaseDatapath::prepareForScheduling() {
  std::cout << "=============================================" << std::endl;
  std::cout << "      Scheduling...            " << benchName << std::endl;
  std::cout << "=============================================" << std::endl;

  edgeToParid = get(boost::edge_name, program.graph);

  numTotalEdges = boost::num_edges(program.graph);
  executedNodes = 0;
  totalConnectedNodes = 0;
  for (auto node_it = program.nodes.begin(); node_it != program.nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (!node->has_vertex())
      continue;
    Vertex node_vertex = node->get_vertex();
    if (boost::degree(node_vertex, program.graph) != 0 || node->is_dma_load() ||
        node->is_dma_store() || node->is_dma_fence()) {
      node->set_num_parents(boost::in_degree(node_vertex, program.graph));
      node->set_isolated(false);
      totalConnectedNodes++;
    }
  }
  std::cout << "  Total connected nodes: " << totalConnectedNodes << "\n";
  std::cout << "  Total edges: " << numTotalEdges << "\n";
  std::cout << "=============================================" << std::endl;

  executingQueue.clear();
  readyToExecuteQueue.clear();
  initExecutingQueue();
}

void BaseDatapath::dumpGraph(std::string graph_name) {
  std::unordered_map<Vertex, ExecNode*> vertexToMicroop;
  BGL_FORALL_VERTICES(v, program.graph, Graph) {
    ExecNode* node = program.nodes.at(get(boost::vertex_node_id, program.graph, v));
    vertexToMicroop[v] = node;
  }
  std::ofstream out(
      graph_name + "_graph.dot", std::ofstream::out | std::ofstream::app);
  write_graphviz(out, program.graph,
                 make_microop_label_writer(vertexToMicroop, program.graph));
}

/*As Late As Possible (ALAP) rescheduling for non-memory, non-control nodes.
  The first pass of scheduling is as early as possible, whenever a node's
  parents are ready, the node is executed. This mode of executing potentially
  can lead to values that are produced way earlier than they are needed. For
  such case, we add an ALAP rescheduling pass to reorganize the graph without
  changing the critical path and memory nodes, but produce a more balanced
  design.*/
int BaseDatapath::rescheduleNodesWhenNeeded() {
  std::vector<Vertex> topo_nodes;
  boost::topological_sort(program.graph, std::back_inserter(topo_nodes));
  // bottom nodes first
  std::map<unsigned, int> earliest_child;
  for (auto node_id_pair : program.nodes) {
    earliest_child[node_id_pair.first] = num_cycles;
  }
  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi) {
    unsigned node_id = program.atVertex(*vi);
    ExecNode* node = program.nodes.at(node_id);
    if (node->is_isolated())
      continue;
    if (!node->is_memory_op() && !node->is_branch_op()) {
      int new_cycle = earliest_child.at(node_id) - 1;
      if (new_cycle > node->get_complete_execution_cycle()) {
        node->set_complete_execution_cycle(new_cycle);
        if (node->is_fp_op()) {
          node->set_start_execution_cycle(
              new_cycle - node->fp_node_latency_in_cycles() + 1);
        } else {
          node->set_start_execution_cycle(new_cycle);
        }
      }
    }

    in_edge_iter in_i, in_end;
    for (boost::tie(in_i, in_end) = in_edges(*vi, program.graph); in_i != in_end;
         ++in_i) {
      int parent_id = program.atVertex(source(*in_i, program.graph));
      if (earliest_child.at(parent_id) > node->get_start_execution_cycle())
        earliest_child.at(parent_id) = node->get_start_execution_cycle();
    }
  }
  return num_cycles;
}

void BaseDatapath::computeRegStats() {
  regStats.assign(num_cycles, { 0, 0 });
  for (auto node_it = program.nodes.begin(); node_it != program.nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (node->is_isolated() || node->is_control_op() || node->is_index_op())
      continue;
    int node_level = node->get_complete_execution_cycle();
    int max_children_level = node_level;

    Vertex node_vertex = node->get_vertex();
    out_edge_iter out_edge_it, out_edge_end;
    std::set<int> children_levels;
    for (boost::tie(out_edge_it, out_edge_end) =
             out_edges(node_vertex, program.graph);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      int child_id = program.atVertex(target(*out_edge_it, program.graph));
      ExecNode* child_node = program.nodes.at(child_id);
      if (child_node->is_control_op() || child_node->is_load_op() ||
          child_node->is_call_op() || child_node->is_ret_op() ||
          child_node->is_convert_op())
        continue;

      int child_level = child_node->get_start_execution_cycle();
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
    ExecNode* node = *it;
    if (node->is_store_op())
      executingQueue.push_front(node);
    else
      executingQueue.push_back(node);
    it = readyToExecuteQueue.erase(it);
  }
}

bool BaseDatapath::step() {
  stepExecutingQueue();
  copyToExecutingQueue();
  num_cycles++;
  if (executedNodes == totalConnectedNodes)
    return true;
  return false;
}

void BaseDatapath::markNodeStarted(ExecNode* node) {
  node->set_start_execution_cycle(num_cycles);
}

// Marks a node as completed and advances the executing queue iterator.
void BaseDatapath::markNodeCompleted(
    std::list<ExecNode*>::iterator& executingQueuePos, int& advance_to) {
  ExecNode* node = *executingQueuePos;
  executedNodes++;
  node->set_complete_execution_cycle(num_cycles);
  executingQueue.erase(executingQueuePos);
  updateChildren(node);
  executingQueuePos = executingQueue.begin();
  std::advance(executingQueuePos, advance_to);
}

void BaseDatapath::updateChildren(ExecNode* node) {
  if (!node->has_vertex())
    return;
  Vertex node_vertex = node->get_vertex();
  out_edge_iter out_edge_it, out_edge_end;
  for (boost::tie(out_edge_it, out_edge_end) = out_edges(node_vertex, program.graph);
       out_edge_it != out_edge_end;
       ++out_edge_it) {
    Vertex child_vertex = target(*out_edge_it, program.graph);
    ExecNode* child_node = program.nodeAtVertex(child_vertex);
    int edge_parid = edgeToParid[*out_edge_it];
    if (child_node->get_num_parents() > 0) {
      child_node->decr_num_parents();
      if (child_node->get_num_parents() == 0) {
        bool child_zero_latency =
            (child_node->is_memory_op())
                ? false
                : (child_node->fu_node_latency(user_params.cycle_time) == 0);
        bool curr_zero_latency =
            (node->is_memory_op())
                ? false
                : (node->fu_node_latency(user_params.cycle_time) == 0);
        if (edge_parid == REGISTER_EDGE || edge_parid == FUSED_BRANCH_EDGE ||
            ((child_zero_latency || curr_zero_latency) &&
             edge_parid != CONTROL_EDGE)) {
          executingQueue.push_back(child_node);
        } else {
          if (child_node->is_store_op())
            readyToExecuteQueue.push_front(child_node);
          else
            readyToExecuteQueue.push_back(child_node);
        }
        child_node->set_num_parents(-1);
      }
    }
  }
}

void BaseDatapath::initExecutingQueue() {
  for (auto node_it = program.nodes.begin(); node_it != program.nodes.end();
       ++node_it) {
    ExecNode* node = node_it->second;
    if (node->get_num_parents() == 0 && !node->is_isolated())
      executingQueue.push_back(node);
  }
}

// readConfigs
void BaseDatapath::parse_config(std::string& bench,
                                std::string& config_file_name) {
  std::ifstream config_file;
  config_file.open(config_file_name, std::ifstream::in);
  if (config_file.fail()) {
    std::cerr
        << "[ERROR]: Failed to open the configuration file " << config_file_name
        << ". Please verify the file exists and permissions are correct.\n";
    exit(1);
  }
  std::string wholeline;

  while (!config_file.eof()) {
    wholeline.clear();
    std::getline(config_file, wholeline);
    if (wholeline.size() == 0)  // Empty line.
      continue;
    if (wholeline[0] == '#')  // Comment.
      continue;
    std::string type, rest_line;
    int pos_end_tag = wholeline.find(",");
    if (pos_end_tag == -1)
      break;
    type = wholeline.substr(0, pos_end_tag);
    rest_line = wholeline.substr(pos_end_tag + 1);
    if (!type.compare("flatten")) {
      char function_name[256], label_or_line_num[64];
      sscanf(
          rest_line.c_str(), "%[^,],%[^,]\n", function_name, label_or_line_num);
      Function* function = srcManager.insert<Function>(function_name);
      Label* label = srcManager.insert<Label>(label_or_line_num);
      UniqueLabel unrolling_id(function, label);
      user_params.unrolling[unrolling_id] = 0;
    } else if (!type.compare("unrolling")) {
      char function_name[256], label_or_line_num[64];
      int factor;
      sscanf(rest_line.c_str(), "%[^,],%[^,],%d\n", function_name,
             label_or_line_num, &factor);
      Function* function = srcManager.insert<Function>(function_name);
      Label* label = srcManager.insert<Label>(label_or_line_num);
      UniqueLabel unrolling_id(function, label);
      user_params.unrolling[unrolling_id] = factor;
    } else if (!type.compare("partition")) {
      unsigned size = 0, p_factor = 0, wordsize = 0;
      char part_type[256];
      char array_label[256];
      PartitionType p_type;
      MemoryType m_type;
      if (wholeline.find("complete") == std::string::npos) {
        sscanf(rest_line.c_str(),
               "%[^,],%[^,],%d,%d,%d\n",
               part_type,
               array_label,
               &size,
               &wordsize,
               &p_factor);
        m_type = spad;
        if (strncmp(part_type, "cyclic", 6) == 0)
          p_type = cyclic;
        else
          p_type = block;
      } else {
        sscanf(rest_line.c_str(),
               "%[^,],%[^,],%d\n",
               part_type,
               array_label,
               &size);
        p_type = complete;
        m_type = reg;
      }
      user_params.partition[array_label] = {
        m_type, p_type, size, wordsize, p_factor, 0
      };
    } else if (!type.compare("cache")) {
      unsigned size = 0, p_factor = 0, wordsize = 0;
      char array_label[256];
      sscanf(rest_line.c_str(), "%[^,],%d\n", array_label, &size);
      std::string p_type(type);
      user_params.partition[array_label] = {
        cache, none, size, wordsize, p_factor, 0
      };
    } else if (!type.compare("acp")) {
      char array_label[256];
      sscanf(rest_line.c_str(), "%[^,]\n", array_label);
      std::string p_type(type);
      user_params.partition[array_label] = { acp, none, 0, 0, 0, 0 };
    } else if (!type.compare("pipelining")) {
      user_params.global_pipelining = atoi(rest_line.c_str());
    } else if (!type.compare("pipeline")) {
      char function_name[256], label_or_line_num[64];
      sscanf(rest_line.c_str(), "%[^,],%[^,]\n", function_name,
             label_or_line_num);
      Function* function = srcManager.insert<Function>(function_name);
      Label* label = srcManager.insert<Label>(label_or_line_num);
      UniqueLabel pipeline_id(function, label);
      if (user_params.pipeline.find(pipeline_id) ==
          user_params.pipeline.end()) {
        user_params.pipeline.insert(pipeline_id);
      } else {
        std::cerr << "Duplicate pipeline directive for " << function_name << "/"
                  << label_or_line_num << ", ignoring.\n";
      }
    } else if (!type.compare("cycle_time")) {
      // Update the global cycle time parameter.
      user_params.cycle_time = stof(rest_line);
    } else if (!type.compare("ready_mode")) {
      user_params.ready_mode = atoi(rest_line.c_str());
    } else if (!type.compare("scratchpad_ports")) {
      user_params.scratchpad_ports = atoi(rest_line.c_str());
    } else {
      std::cerr << "Invalid config type: " << wholeline << std::endl;
      exit(1);
    }
  }
  config_file.close();
}

#ifdef USE_DB
void BaseDatapath::setExperimentParameters(std::string experiment_name) {
  use_db = true;
  this->experiment_name = experiment_name;
}

void BaseDatapath::getCommonConfigParameters(int& unrolling_factor,
                                             bool& pipelining_factor,
                                             int& partition_factor) {
  // First, collect pipelining, unrolling, and partitioning parameters. We'll
  // assume that all parameters are uniform for all loops, and we'll insert a
  // path to the actual config file if we need to look up the actual
  // configurations.
  unrolling_factor =
      user_params.unrolling.empty() ? 1 : user_params.unrolling.begin()->second;
  pipelining_factor = pipelining;
  partition_factor = user_params.partition.empty()
                         ? 1
                         : user_params.partition.begin()->second.part_factor;
}

/* Returns the experiment_id for the experiment_name. If the experiment_name
 * does not exist in the database, then a new experiment_id is created and
 * inserted into the database. */
int BaseDatapath::getExperimentId(sql::Connection* con) {
  sql::ResultSet* res;
  sql::Statement* stmt = con->createStatement();
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
    query.str("");  // Clear stringstream.
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

int BaseDatapath::getLastInsertId(sql::Connection* con) {
  sql::Statement* stmt = con->createStatement();
  sql::ResultSet* res = stmt->executeQuery("select last_insert_id()");
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

void BaseDatapath::writeSummaryToDatabase(summary_data_t& summary) {
  sql::Driver* driver;
  sql::Connection* con;
  sql::Statement* stmt;
  driver = sql::mysql::get_mysql_driver_instance();
  con = driver->connect(DB_URL, DB_USER, DB_PASS);
  con->setSchema("aladdin");
  con->setAutoCommit(0);  // Begin transaction.
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
  con->commit();  // End transaction.
  delete stmt;
  delete con;
}
#endif
