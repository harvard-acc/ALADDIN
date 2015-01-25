#include "power_func.h"

void getRegisterPowerArea(float cycle_time, float *internal_power_per_bit,
                          float *switch_power_per_bit,
                          float *leakage_power_per_bit,
                          float *area_per_bit)
{
  switch ((int)cycle_time) { //cycleTime in ns
    case 6:
      *internal_power_per_bit    = REG_6ns_int_power;
      *switch_power_per_bit      = REG_6ns_switch_power;
      *leakage_power_per_bit     = REG_6ns_leakage_power;
      *area_per_bit              = REG_6ns_area;
      break;
    case 5:
      *internal_power_per_bit    = REG_5ns_int_power;
      *switch_power_per_bit      = REG_5ns_switch_power;
      *leakage_power_per_bit     = REG_5ns_leakage_power;
      *area_per_bit              = REG_5ns_area;
      break;
    case 4:
      *internal_power_per_bit    = REG_4ns_int_power;
      *switch_power_per_bit      = REG_4ns_switch_power;
      *leakage_power_per_bit     = REG_4ns_leakage_power;
      *area_per_bit              = REG_4ns_area;
      break;
    case 3:
      *internal_power_per_bit    = REG_3ns_int_power;
      *switch_power_per_bit      = REG_3ns_switch_power;
      *leakage_power_per_bit     = REG_3ns_leakage_power;
      *area_per_bit              = REG_3ns_area;
      break;
    case 2:
      *internal_power_per_bit    = REG_2ns_int_power;
      *switch_power_per_bit      = REG_2ns_switch_power;
      *leakage_power_per_bit     = REG_2ns_leakage_power;
      *area_per_bit              = REG_2ns_area;
      break;
    case 1:
      *internal_power_per_bit    = REG_1ns_int_power;
      *switch_power_per_bit      = REG_1ns_switch_power;
      *leakage_power_per_bit     = REG_1ns_leakage_power;
      *area_per_bit              = REG_1ns_area;
      break;
    default:
      std::cerr << " Current power model supports acclerators running"
                << " at 1, 2, 3, 4, 5 and 6 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << std::endl;
      exit(1);
  }
}
void getAdderPowerArea(float cycle_time, float *internal_power,
                       float *switch_power, float *leakage_power,
                       float *area)
{
  switch ((int)cycle_time) { //cycle_time in ns
    case 6:
      *internal_power    = ADD_6ns_int_power;
      *switch_power      = ADD_6ns_switch_power;
      *leakage_power     = ADD_6ns_leakage_power;
      *area              = ADD_6ns_area;
      break;
    case 5:
      *internal_power    = ADD_5ns_int_power;
      *switch_power      = ADD_5ns_switch_power;
      *leakage_power     = ADD_5ns_leakage_power;
      *area              = ADD_5ns_area;
      break;
    case 4:
      *internal_power    = ADD_4ns_int_power;
      *switch_power      = ADD_4ns_switch_power;
      *leakage_power     = ADD_4ns_leakage_power;
      *area              = ADD_4ns_area;
      break;
    case 3:
      *internal_power    = ADD_3ns_int_power;
      *switch_power      = ADD_3ns_switch_power;
      *leakage_power     = ADD_3ns_leakage_power;
      *area              = ADD_3ns_area;
      break;
    case 2:
      *internal_power    = ADD_2ns_int_power;
      *switch_power      = ADD_2ns_switch_power;
      *leakage_power     = ADD_2ns_leakage_power;
      *area              = ADD_2ns_area;
      break;
    case 1:
      *internal_power    = ADD_1ns_int_power;
      *switch_power      = ADD_1ns_switch_power;
      *leakage_power     = ADD_1ns_leakage_power;
      *area              = ADD_1ns_area;
      break;
    default:
      std::cerr << " Current power model supports acclerators running"
                << " at 1, 2, 3, 4, 5 and 6 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << std::endl;
      exit(1);
  }
}
void getMultiplierPowerArea(float cycle_time, float *internal_power,
                            float *switch_power, float *leakage_power,
                            float *area)
{
  switch ((int)cycle_time) { //cycle_time in ns
    case 6:
      *internal_power    = MUL_6ns_int_power;
      *switch_power      = MUL_6ns_switch_power;
      *leakage_power     = MUL_6ns_leakage_power;
      *area              = MUL_6ns_area;
      break;
    case 5:
      *internal_power    = MUL_5ns_int_power;
      *switch_power      = MUL_5ns_switch_power;
      *leakage_power     = MUL_5ns_leakage_power;
      *area              = MUL_5ns_area;
      break;
    case 4:
      *internal_power    = MUL_4ns_int_power;
      *switch_power      = MUL_4ns_switch_power;
      *leakage_power     = MUL_4ns_leakage_power;
      *area              = MUL_4ns_area;
      break;
    case 3:
      *internal_power    = MUL_3ns_int_power;
      *switch_power      = MUL_3ns_switch_power;
      *leakage_power     = MUL_3ns_leakage_power;
      *area              = MUL_3ns_area;
      break;
    case 2:
      *internal_power    = MUL_2ns_int_power;
      *switch_power      = MUL_2ns_switch_power;
      *leakage_power     = MUL_2ns_leakage_power;
      *area              = MUL_2ns_area;
      break;
    case 1:
      *internal_power    = MUL_1ns_int_power;
      *switch_power      = MUL_1ns_switch_power;
      *leakage_power     = MUL_1ns_leakage_power;
      *area              = MUL_1ns_area;
      break;
    default:
      std::cerr << " Current power model supports acclerators running"
                << " at 1, 2, 3, 4, 5 and 6 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << std::endl;
      exit(1);
  }
}
void getBitPowerArea(float cycle_time, float *internal_power,
                            float *switch_power, float *leakage_power,
                            float *area)
{
  switch ((int)cycle_time) { //cycle_time in ns
    case 6:
      *internal_power    = BIT_6ns_int_power;
      *switch_power      = BIT_6ns_switch_power;
      *leakage_power     = BIT_6ns_leakage_power;
      *area              = BIT_6ns_area;
      break;
    case 5:
      *internal_power    = BIT_5ns_int_power;
      *switch_power      = BIT_5ns_switch_power;
      *leakage_power     = BIT_5ns_leakage_power;
      *area              = BIT_5ns_area;
      break;
    case 4:
      *internal_power    = BIT_4ns_int_power;
      *switch_power      = BIT_4ns_switch_power;
      *leakage_power     = BIT_4ns_leakage_power;
      *area              = BIT_4ns_area;
      break;
    case 3:
      *internal_power    = BIT_3ns_int_power;
      *switch_power      = BIT_3ns_switch_power;
      *leakage_power     = BIT_3ns_leakage_power;
      *area              = BIT_3ns_area;
      break;
    case 2:
      *internal_power    = BIT_2ns_int_power;
      *switch_power      = BIT_2ns_switch_power;
      *leakage_power     = BIT_2ns_leakage_power;
      *area              = BIT_2ns_area;
      break;
    case 1:
      *internal_power    = BIT_1ns_int_power;
      *switch_power      = BIT_1ns_switch_power;
      *leakage_power     = BIT_1ns_leakage_power;
      *area              = BIT_1ns_area;
      break;
    default:
      std::cerr << " Current power model supports acclerators running"
                << " at 1, 2, 3, 4, 5 and 6 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << std::endl;
      exit(1);
  }
}
void getShifterPowerArea(float cycle_time, float *internal_power,
                            float *switch_power, float *leakage_power,
                            float *area)
{
  switch ((int)cycle_time) { //cycle_time in ns
    case 6:
      *internal_power    = SHIFTER_6ns_int_power;
      *switch_power      = SHIFTER_6ns_switch_power;
      *leakage_power     = SHIFTER_6ns_leakage_power;
      *area              = SHIFTER_6ns_area;
      break;
    case 5:
      *internal_power    = SHIFTER_5ns_int_power;
      *switch_power      = SHIFTER_5ns_switch_power;
      *leakage_power     = SHIFTER_5ns_leakage_power;
      *area              = SHIFTER_5ns_area;
      break;
    case 4:
      *internal_power    = SHIFTER_4ns_int_power;
      *switch_power      = SHIFTER_4ns_switch_power;
      *leakage_power     = SHIFTER_4ns_leakage_power;
      *area              = SHIFTER_4ns_area;
      break;
    case 3:
      *internal_power    = SHIFTER_3ns_int_power;
      *switch_power      = SHIFTER_3ns_switch_power;
      *leakage_power     = SHIFTER_3ns_leakage_power;
      *area              = SHIFTER_3ns_area;
      break;
    case 2:
      *internal_power    = SHIFTER_2ns_int_power;
      *switch_power      = SHIFTER_2ns_switch_power;
      *leakage_power     = SHIFTER_2ns_leakage_power;
      *area              = SHIFTER_2ns_area;
      break;
    case 1:
      *internal_power    = SHIFTER_1ns_int_power;
      *switch_power      = SHIFTER_1ns_switch_power;
      *leakage_power     = SHIFTER_1ns_leakage_power;
      *area              = SHIFTER_1ns_area;
      break;
    default:
      std::cerr << " Current power model supports acclerators running"
                << " at 1, 2, 3, 4, 5 and 6 ns. " << std::endl;
      std::cerr << " Cycle time: " << cycle_time << " is not supported yet."
                << std::endl;
      exit(1);
  }
}
