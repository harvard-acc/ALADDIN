#include "power_func.h"

void getRegisterPowerArea(float cycle_time,
                          float* internal_power_per_bit,
                          float* switch_power_per_bit,
                          float* leakage_power_per_bit,
                          float* area_per_bit) {
  switch ((int)cycle_time) {  // cycleTime in ns
    case 10:
      *internal_power_per_bit = REG_10ns_int_power;
      *switch_power_per_bit = REG_10ns_switch_power;
      *leakage_power_per_bit = REG_10ns_leakage_power;
      *area_per_bit = REG_10ns_area;
      break;
    case 6:
      *internal_power_per_bit = REG_6ns_int_power;
      *switch_power_per_bit = REG_6ns_switch_power;
      *leakage_power_per_bit = REG_6ns_leakage_power;
      *area_per_bit = REG_6ns_area;
      break;
    case 5:
      *internal_power_per_bit = REG_5ns_int_power;
      *switch_power_per_bit = REG_5ns_switch_power;
      *leakage_power_per_bit = REG_5ns_leakage_power;
      *area_per_bit = REG_5ns_area;
      break;
    case 4:
      *internal_power_per_bit = REG_4ns_int_power;
      *switch_power_per_bit = REG_4ns_switch_power;
      *leakage_power_per_bit = REG_4ns_leakage_power;
      *area_per_bit = REG_4ns_area;
      break;
    case 3:
      *internal_power_per_bit = REG_3ns_int_power;
      *switch_power_per_bit = REG_3ns_switch_power;
      *leakage_power_per_bit = REG_3ns_leakage_power;
      *area_per_bit = REG_3ns_area;
      break;
    case 2:
      *internal_power_per_bit = REG_2ns_int_power;
      *switch_power_per_bit = REG_2ns_switch_power;
      *leakage_power_per_bit = REG_2ns_leakage_power;
      *area_per_bit = REG_2ns_area;
      break;
    case 1:
      *internal_power_per_bit = REG_1ns_int_power;
      *switch_power_per_bit = REG_1ns_switch_power;
      *leakage_power_per_bit = REG_1ns_leakage_power;
      *area_per_bit = REG_1ns_area;
      break;
    default:
      std::cerr << " Current power model supports accelerators running"
                << " at 1, 2, 3, 4, 5, 6, and 10 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << " Use 6ns power model instead." << std::endl;
      *internal_power_per_bit = REG_6ns_int_power;
      *switch_power_per_bit = REG_6ns_switch_power;
      *leakage_power_per_bit = REG_6ns_leakage_power;
      *area_per_bit = REG_6ns_area;
      break;
  }
}
void getAdderPowerArea(float cycle_time,
                       float* internal_power,
                       float* switch_power,
                       float* leakage_power,
                       float* area) {
  switch ((int)cycle_time) {  // cycle_time in ns
    case 10:
      *internal_power = ADD_10ns_int_power;
      *switch_power = ADD_10ns_switch_power;
      *leakage_power = ADD_10ns_leakage_power;
      *area = ADD_10ns_area;
      break;
    case 6:
      *internal_power = ADD_6ns_int_power;
      *switch_power = ADD_6ns_switch_power;
      *leakage_power = ADD_6ns_leakage_power;
      *area = ADD_6ns_area;
      break;
    case 5:
      *internal_power = ADD_5ns_int_power;
      *switch_power = ADD_5ns_switch_power;
      *leakage_power = ADD_5ns_leakage_power;
      *area = ADD_5ns_area;
      break;
    case 4:
      *internal_power = ADD_4ns_int_power;
      *switch_power = ADD_4ns_switch_power;
      *leakage_power = ADD_4ns_leakage_power;
      *area = ADD_4ns_area;
      break;
    case 3:
      *internal_power = ADD_3ns_int_power;
      *switch_power = ADD_3ns_switch_power;
      *leakage_power = ADD_3ns_leakage_power;
      *area = ADD_3ns_area;
      break;
    case 2:
      *internal_power = ADD_2ns_int_power;
      *switch_power = ADD_2ns_switch_power;
      *leakage_power = ADD_2ns_leakage_power;
      *area = ADD_2ns_area;
      break;
    case 1:
      *internal_power = ADD_1ns_int_power;
      *switch_power = ADD_1ns_switch_power;
      *leakage_power = ADD_1ns_leakage_power;
      *area = ADD_1ns_area;
      break;
    default:
      std::cerr << " Current power model supports accelerators running"
                << " at 1, 2, 3, 4, 5, 6, and 10 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << " Use 6ns power model instead." << std::endl;
      *internal_power = ADD_6ns_int_power;
      *switch_power = ADD_6ns_switch_power;
      *leakage_power = ADD_6ns_leakage_power;
      *area = ADD_6ns_area;
      break;
  }
}
void getMultiplierPowerArea(float cycle_time,
                            float* internal_power,
                            float* switch_power,
                            float* leakage_power,
                            float* area) {
  switch ((int)cycle_time) {  // cycle_time in ns
    case 10:
      *internal_power = MUL_10ns_int_power;
      *switch_power = MUL_10ns_switch_power;
      *leakage_power = MUL_10ns_leakage_power;
      *area = MUL_10ns_area;
      break;
    case 6:
      *internal_power = MUL_6ns_int_power;
      *switch_power = MUL_6ns_switch_power;
      *leakage_power = MUL_6ns_leakage_power;
      *area = MUL_6ns_area;
      break;
    case 5:
      *internal_power = MUL_5ns_int_power;
      *switch_power = MUL_5ns_switch_power;
      *leakage_power = MUL_5ns_leakage_power;
      *area = MUL_5ns_area;
      break;
    case 4:
      *internal_power = MUL_4ns_int_power;
      *switch_power = MUL_4ns_switch_power;
      *leakage_power = MUL_4ns_leakage_power;
      *area = MUL_4ns_area;
      break;
    case 3:
      *internal_power = MUL_3ns_int_power;
      *switch_power = MUL_3ns_switch_power;
      *leakage_power = MUL_3ns_leakage_power;
      *area = MUL_3ns_area;
      break;
    case 2:
      *internal_power = MUL_2ns_int_power;
      *switch_power = MUL_2ns_switch_power;
      *leakage_power = MUL_2ns_leakage_power;
      *area = MUL_2ns_area;
      break;
    case 1:
      *internal_power = MUL_1ns_int_power;
      *switch_power = MUL_1ns_switch_power;
      *leakage_power = MUL_1ns_leakage_power;
      *area = MUL_1ns_area;
      break;
    default:
      std::cerr << " Current power model supports accelerators running"
                << " at 1, 2, 3, 4, 5, 6, and 10 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << " Use 6ns power model instead." << std::endl;
      *internal_power = MUL_6ns_int_power;
      *switch_power = MUL_6ns_switch_power;
      *leakage_power = MUL_6ns_leakage_power;
      *area = MUL_6ns_area;
      break;
  }
}
void getBitPowerArea(float cycle_time,
                     float* internal_power,
                     float* switch_power,
                     float* leakage_power,
                     float* area) {
  switch ((int)cycle_time) {  // cycle_time in ns
    case 10:
      *internal_power = BIT_10ns_int_power;
      *switch_power = BIT_10ns_switch_power;
      *leakage_power = BIT_10ns_leakage_power;
      *area = BIT_10ns_area;
      break;
    case 6:
      *internal_power = BIT_6ns_int_power;
      *switch_power = BIT_6ns_switch_power;
      *leakage_power = BIT_6ns_leakage_power;
      *area = BIT_6ns_area;
      break;
    case 5:
      *internal_power = BIT_5ns_int_power;
      *switch_power = BIT_5ns_switch_power;
      *leakage_power = BIT_5ns_leakage_power;
      *area = BIT_5ns_area;
      break;
    case 4:
      *internal_power = BIT_4ns_int_power;
      *switch_power = BIT_4ns_switch_power;
      *leakage_power = BIT_4ns_leakage_power;
      *area = BIT_4ns_area;
      break;
    case 3:
      *internal_power = BIT_3ns_int_power;
      *switch_power = BIT_3ns_switch_power;
      *leakage_power = BIT_3ns_leakage_power;
      *area = BIT_3ns_area;
      break;
    case 2:
      *internal_power = BIT_2ns_int_power;
      *switch_power = BIT_2ns_switch_power;
      *leakage_power = BIT_2ns_leakage_power;
      *area = BIT_2ns_area;
      break;
    case 1:
      *internal_power = BIT_1ns_int_power;
      *switch_power = BIT_1ns_switch_power;
      *leakage_power = BIT_1ns_leakage_power;
      *area = BIT_1ns_area;
      break;
    default:
      std::cerr << " Current power model supports accelerators running"
                << " at 1, 2, 3, 4, 5, 6, and 10 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << " Use 6ns power model instead." << std::endl;
      *internal_power = BIT_6ns_int_power;
      *switch_power = BIT_6ns_switch_power;
      *leakage_power = BIT_6ns_leakage_power;
      *area = BIT_6ns_area;
      break;
  }
}
void getShifterPowerArea(float cycle_time,
                         float* internal_power,
                         float* switch_power,
                         float* leakage_power,
                         float* area) {
  switch ((int)cycle_time) {  // cycle_time in ns
    case 10:
      *internal_power = SHIFTER_10ns_int_power;
      *switch_power = SHIFTER_10ns_switch_power;
      *leakage_power = SHIFTER_10ns_leakage_power;
      *area = SHIFTER_10ns_area;
      break;
    case 6:
      *internal_power = SHIFTER_6ns_int_power;
      *switch_power = SHIFTER_6ns_switch_power;
      *leakage_power = SHIFTER_6ns_leakage_power;
      *area = SHIFTER_6ns_area;
      break;
    case 5:
      *internal_power = SHIFTER_5ns_int_power;
      *switch_power = SHIFTER_5ns_switch_power;
      *leakage_power = SHIFTER_5ns_leakage_power;
      *area = SHIFTER_5ns_area;
      break;
    case 4:
      *internal_power = SHIFTER_4ns_int_power;
      *switch_power = SHIFTER_4ns_switch_power;
      *leakage_power = SHIFTER_4ns_leakage_power;
      *area = SHIFTER_4ns_area;
      break;
    case 3:
      *internal_power = SHIFTER_3ns_int_power;
      *switch_power = SHIFTER_3ns_switch_power;
      *leakage_power = SHIFTER_3ns_leakage_power;
      *area = SHIFTER_3ns_area;
      break;
    case 2:
      *internal_power = SHIFTER_2ns_int_power;
      *switch_power = SHIFTER_2ns_switch_power;
      *leakage_power = SHIFTER_2ns_leakage_power;
      *area = SHIFTER_2ns_area;
      break;
    case 1:
      *internal_power = SHIFTER_1ns_int_power;
      *switch_power = SHIFTER_1ns_switch_power;
      *leakage_power = SHIFTER_1ns_leakage_power;
      *area = SHIFTER_1ns_area;
      break;
    default:
      std::cerr << " Current power model supports accelerators running"
                << " at 1, 2, 3, 4, 5, 6, and 10 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << " Use 6ns power model instead." << std::endl;
      *internal_power = SHIFTER_6ns_int_power;
      *switch_power = SHIFTER_6ns_switch_power;
      *leakage_power = SHIFTER_6ns_leakage_power;
      *area = SHIFTER_6ns_area;
      break;
  }
}
void getSinglePrecisionFloatingPointAdderPowerArea(float cycle_time,
                                                   float* internal_power,
                                                   float* switch_power,
                                                   float* leakage_power,
                                                   float* area) {
  switch ((int)cycle_time) {  // cycle_time in ns
    case 10:
      *internal_power = FP_SP_3STAGE_ADD_10ns_int_power;
      *switch_power = FP_SP_3STAGE_ADD_10ns_switch_power;
      *leakage_power = FP_SP_3STAGE_ADD_10ns_leakage_power;
      *area = FP_SP_3STAGE_ADD_10ns_area;
      break;
    case 6:
      *internal_power = FP_SP_3STAGE_ADD_6ns_int_power;
      *switch_power = FP_SP_3STAGE_ADD_6ns_switch_power;
      *leakage_power = FP_SP_3STAGE_ADD_6ns_leakage_power;
      *area = FP_SP_3STAGE_ADD_6ns_area;
      break;
    case 5:
      *internal_power = FP_SP_3STAGE_ADD_5ns_int_power;
      *switch_power = FP_SP_3STAGE_ADD_5ns_switch_power;
      *leakage_power = FP_SP_3STAGE_ADD_5ns_leakage_power;
      *area = FP_SP_3STAGE_ADD_5ns_area;
      break;
    case 4:
      *internal_power = FP_SP_3STAGE_ADD_4ns_int_power;
      *switch_power = FP_SP_3STAGE_ADD_4ns_switch_power;
      *leakage_power = FP_SP_3STAGE_ADD_4ns_leakage_power;
      *area = FP_SP_3STAGE_ADD_4ns_area;
      break;
    case 3:
      *internal_power = FP_SP_3STAGE_ADD_3ns_int_power;
      *switch_power = FP_SP_3STAGE_ADD_3ns_switch_power;
      *leakage_power = FP_SP_3STAGE_ADD_3ns_leakage_power;
      *area = FP_SP_3STAGE_ADD_3ns_area;
      break;
    case 2:
      *internal_power = FP_SP_3STAGE_ADD_2ns_int_power;
      *switch_power = FP_SP_3STAGE_ADD_2ns_switch_power;
      *leakage_power = FP_SP_3STAGE_ADD_2ns_leakage_power;
      *area = FP_SP_3STAGE_ADD_2ns_area;
      break;
    case 1:
      *internal_power = FP_SP_3STAGE_ADD_1ns_int_power;
      *switch_power = FP_SP_3STAGE_ADD_1ns_switch_power;
      *leakage_power = FP_SP_3STAGE_ADD_1ns_leakage_power;
      *area = FP_SP_3STAGE_ADD_1ns_area;
      break;
    default:
      std::cerr << " Current power model supports accelerators running"
                << " at 1, 2, 3, 4, 5, 6, and 10 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << " Use 6ns power model instead." << std::endl;
      *internal_power = FP_SP_3STAGE_ADD_6ns_int_power;
      *switch_power = FP_SP_3STAGE_ADD_6ns_switch_power;
      *leakage_power = FP_SP_3STAGE_ADD_6ns_leakage_power;
      *area = FP_SP_3STAGE_ADD_6ns_area;
      break;
  }
}
void getDoublePrecisionFloatingPointAdderPowerArea(float cycle_time,
                                                   float* internal_power,
                                                   float* switch_power,
                                                   float* leakage_power,
                                                   float* area) {
  switch ((int)cycle_time) {  // cycle_time in ns
    case 10:
      *internal_power = FP_DP_3STAGE_ADD_10ns_int_power;
      *switch_power = FP_DP_3STAGE_ADD_10ns_switch_power;
      *leakage_power = FP_DP_3STAGE_ADD_10ns_leakage_power;
      *area = FP_DP_3STAGE_ADD_10ns_area;
      break;
    case 6:
      *internal_power = FP_DP_3STAGE_ADD_6ns_int_power;
      *switch_power = FP_DP_3STAGE_ADD_6ns_switch_power;
      *leakage_power = FP_DP_3STAGE_ADD_6ns_leakage_power;
      *area = FP_DP_3STAGE_ADD_6ns_area;
      break;
    case 5:
      *internal_power = FP_DP_3STAGE_ADD_5ns_int_power;
      *switch_power = FP_DP_3STAGE_ADD_5ns_switch_power;
      *leakage_power = FP_DP_3STAGE_ADD_5ns_leakage_power;
      *area = FP_DP_3STAGE_ADD_5ns_area;
      break;
    case 4:
      *internal_power = FP_DP_3STAGE_ADD_4ns_int_power;
      *switch_power = FP_DP_3STAGE_ADD_4ns_switch_power;
      *leakage_power = FP_DP_3STAGE_ADD_4ns_leakage_power;
      *area = FP_DP_3STAGE_ADD_4ns_area;
      break;
    case 3:
      *internal_power = FP_DP_3STAGE_ADD_3ns_int_power;
      *switch_power = FP_DP_3STAGE_ADD_3ns_switch_power;
      *leakage_power = FP_DP_3STAGE_ADD_3ns_leakage_power;
      *area = FP_DP_3STAGE_ADD_3ns_area;
      break;
    case 2:
      *internal_power = FP_DP_3STAGE_ADD_2ns_int_power;
      *switch_power = FP_DP_3STAGE_ADD_2ns_switch_power;
      *leakage_power = FP_DP_3STAGE_ADD_2ns_leakage_power;
      *area = FP_DP_3STAGE_ADD_2ns_area;
      break;
    case 1:
      *internal_power = FP_DP_3STAGE_ADD_1ns_int_power;
      *switch_power = FP_DP_3STAGE_ADD_1ns_switch_power;
      *leakage_power = FP_DP_3STAGE_ADD_1ns_leakage_power;
      *area = FP_DP_3STAGE_ADD_1ns_area;
      break;
    default:
      std::cerr << " Current power model supports accelerators running"
                << " at 1, 2, 3, 4, 5, 6, and 10 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << " Use 6ns power model instead." << std::endl;
      *internal_power = FP_DP_3STAGE_ADD_6ns_int_power;
      *switch_power = FP_DP_3STAGE_ADD_6ns_switch_power;
      *leakage_power = FP_DP_3STAGE_ADD_6ns_leakage_power;
      *area = FP_DP_3STAGE_ADD_6ns_area;
      break;
  }
}
void getSinglePrecisionFloatingPointMultiplierPowerArea(float cycle_time,
                                                        float* internal_power,
                                                        float* switch_power,
                                                        float* leakage_power,
                                                        float* area) {
  switch ((int)cycle_time) {  // cycle_time in ns
    case 10:
      *internal_power = FP_SP_3STAGE_MUL_10ns_int_power;
      *switch_power = FP_SP_3STAGE_MUL_10ns_switch_power;
      *leakage_power = FP_SP_3STAGE_MUL_10ns_leakage_power;
      *area = FP_SP_3STAGE_MUL_10ns_area;
      break;
    case 6:
      *internal_power = FP_SP_3STAGE_MUL_6ns_int_power;
      *switch_power = FP_SP_3STAGE_MUL_6ns_switch_power;
      *leakage_power = FP_SP_3STAGE_MUL_6ns_leakage_power;
      *area = FP_SP_3STAGE_MUL_6ns_area;
      break;
    case 5:
      *internal_power = FP_SP_3STAGE_MUL_5ns_int_power;
      *switch_power = FP_SP_3STAGE_MUL_5ns_switch_power;
      *leakage_power = FP_SP_3STAGE_MUL_5ns_leakage_power;
      *area = FP_SP_3STAGE_MUL_5ns_area;
      break;
    case 4:
      *internal_power = FP_SP_3STAGE_MUL_4ns_int_power;
      *switch_power = FP_SP_3STAGE_MUL_4ns_switch_power;
      *leakage_power = FP_SP_3STAGE_MUL_4ns_leakage_power;
      *area = FP_SP_3STAGE_MUL_4ns_area;
      break;
    case 3:
      *internal_power = FP_SP_3STAGE_MUL_3ns_int_power;
      *switch_power = FP_SP_3STAGE_MUL_3ns_switch_power;
      *leakage_power = FP_SP_3STAGE_MUL_3ns_leakage_power;
      *area = FP_SP_3STAGE_MUL_3ns_area;
      break;
    case 2:
      *internal_power = FP_SP_3STAGE_MUL_2ns_int_power;
      *switch_power = FP_SP_3STAGE_MUL_2ns_switch_power;
      *leakage_power = FP_SP_3STAGE_MUL_2ns_leakage_power;
      *area = FP_SP_3STAGE_MUL_2ns_area;
      break;
    case 1:
      *internal_power = FP_SP_3STAGE_MUL_1ns_int_power;
      *switch_power = FP_SP_3STAGE_MUL_1ns_switch_power;
      *leakage_power = FP_SP_3STAGE_MUL_1ns_leakage_power;
      *area = FP_SP_3STAGE_MUL_1ns_area;
      break;
    default:
      std::cerr << " Current power model supports accelerators running"
                << " at 1, 2, 3, 4, 5, 6, and 10 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << " Use 6ns power model instead." << std::endl;
      *internal_power = FP_SP_3STAGE_MUL_6ns_int_power;
      *switch_power = FP_SP_3STAGE_MUL_6ns_switch_power;
      *leakage_power = FP_SP_3STAGE_MUL_6ns_leakage_power;
      *area = FP_SP_3STAGE_MUL_6ns_area;
      break;
  }
}
void getDoublePrecisionFloatingPointMultiplierPowerArea(float cycle_time,
                                                        float* internal_power,
                                                        float* switch_power,
                                                        float* leakage_power,
                                                        float* area) {
  switch ((int)cycle_time) {  // cycle_time in ns
    case 10:
      *internal_power = FP_DP_3STAGE_MUL_10ns_int_power;
      *switch_power = FP_DP_3STAGE_MUL_10ns_switch_power;
      *leakage_power = FP_DP_3STAGE_MUL_10ns_leakage_power;
      *area = FP_DP_3STAGE_MUL_10ns_area;
      break;
    case 6:
      *internal_power = FP_DP_3STAGE_MUL_6ns_int_power;
      *switch_power = FP_DP_3STAGE_MUL_6ns_switch_power;
      *leakage_power = FP_DP_3STAGE_MUL_6ns_leakage_power;
      *area = FP_DP_3STAGE_MUL_6ns_area;
      break;
    case 5:
      *internal_power = FP_DP_3STAGE_MUL_5ns_int_power;
      *switch_power = FP_DP_3STAGE_MUL_5ns_switch_power;
      *leakage_power = FP_DP_3STAGE_MUL_5ns_leakage_power;
      *area = FP_DP_3STAGE_MUL_5ns_area;
      break;
    case 4:
      *internal_power = FP_DP_3STAGE_MUL_4ns_int_power;
      *switch_power = FP_DP_3STAGE_MUL_4ns_switch_power;
      *leakage_power = FP_DP_3STAGE_MUL_4ns_leakage_power;
      *area = FP_DP_3STAGE_MUL_4ns_area;
      break;
    case 3:
      *internal_power = FP_DP_3STAGE_MUL_3ns_int_power;
      *switch_power = FP_DP_3STAGE_MUL_3ns_switch_power;
      *leakage_power = FP_DP_3STAGE_MUL_3ns_leakage_power;
      *area = FP_DP_3STAGE_MUL_3ns_area;
      break;
    case 2:
      *internal_power = FP_DP_3STAGE_MUL_2ns_int_power;
      *switch_power = FP_DP_3STAGE_MUL_2ns_switch_power;
      *leakage_power = FP_DP_3STAGE_MUL_2ns_leakage_power;
      *area = FP_DP_3STAGE_MUL_2ns_area;
      break;
    case 1:
      *internal_power = FP_DP_3STAGE_MUL_1ns_int_power;
      *switch_power = FP_DP_3STAGE_MUL_1ns_switch_power;
      *leakage_power = FP_DP_3STAGE_MUL_1ns_leakage_power;
      *area = FP_DP_3STAGE_MUL_1ns_area;
      break;
    default:
      std::cerr << " Current power model supports accelerators running"
                << " at 1, 2, 3, 4, 5, 6, and 10 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << " Use 6ns power model instead." << std::endl;
      *internal_power = FP_DP_3STAGE_MUL_6ns_int_power;
      *switch_power = FP_DP_3STAGE_MUL_6ns_switch_power;
      *leakage_power = FP_DP_3STAGE_MUL_6ns_leakage_power;
      *area = FP_DP_3STAGE_MUL_6ns_area;
      break;
  }
}

void getTrigonometricFunctionPowerArea(float cycle_time,
                                      float* internal_power,
                                      float* switch_power,
                                      float* leakage_power,
                                      float* area) {
  switch ((int)cycle_time) {  // cycle_time in ns
    case 10:
      *internal_power = FP_3STAGE_TRIG_10ns_int_power;
      *switch_power   = FP_3STAGE_TRIG_10ns_switch_power;
      *leakage_power  = FP_3STAGE_TRIG_10ns_leakage_power;
      *area           = FP_3STAGE_TRIG_10ns_area;
      break;
    default:
      std::cerr << " Current power model supports trig functions running"
                << " at 10 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << " Use 10ns power model instead." << std::endl;
      *internal_power = FP_3STAGE_TRIG_10ns_int_power;
      *switch_power   = FP_3STAGE_TRIG_10ns_switch_power;
      *leakage_power  = FP_3STAGE_TRIG_10ns_leakage_power;
      *area           = FP_3STAGE_TRIG_10ns_area;
      break;
  }
}

uca_org_t cactiWrapper(unsigned num_of_bytes, unsigned wordsize, unsigned num_ports) {
  int cache_size = num_of_bytes;
  int line_size = wordsize;  // in bytes
  if (wordsize < 4)          // minimum line size in cacti is 32-bit/4-byte
    line_size = 4;
  if (cache_size / line_size < 64)
    cache_size = line_size * 64;  // minimum scratchpad size: 64 words
  int associativity = 1;
  int rw_ports = num_ports;
  if (rw_ports == 0)
    rw_ports = 1;
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
  int obj_func_leakage_power = 100;
  int obj_func_area = 0;
  int obj_func_cycle_time = 0;
  // from CACTI example config...
  int dev_func_delay = 20;
  int dev_func_dynamic_power = 100000;
  int dev_func_leakage_power = 100000;
  int dev_func_area = 1000000;
  int dev_func_cycle_time = 1000000;

  int ed_ed2_none = 2;  // 0 - ED, 1 - ED^2, 2 - use weight and deviate
  int temp = 300;
  int wt = 0;  // 0 - default(search across everything), 1 - global, 2 - 5%
               // delay penalty, 3 - 10%, 4 - 20 %, 5 - 30%, 6 - low-swing
  int data_arr_ram_cell_tech_flavor_in =
      0;  // 0(itrs-hp) 1-itrs-lstp(low standby power)
  int data_arr_peri_global_tech_flavor_in = 0;  // 0(itrs-hp)
  int tag_arr_ram_cell_tech_flavor_in = 0;      // itrs-hp
  int tag_arr_peri_global_tech_flavor_in = 0;   // itrs-hp
  int interconnect_projection_type_in = 1;      // 0 - aggressive, 1 - normal
  int wire_inside_mat_type_in = 1;   // 2 - global, 0 - local, 1 - semi-global
  int wire_outside_mat_type_in = 1;  // 2 - global
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
