#ifndef __POWER_FUNC_H__
#define __POWER_FUNC_H__

#include <stdlib.h>
#include <iostream>

#define   ADD_LATENCY       1
#define   MEMOP_LATENCY     1
#define   MUL_LATENCY       1
#define   RW_PORTS          1
/* All the power numbers are in mW. */

/* Function units power model with TSMC 40g @ 1GHz (1ns). */
#define   ADD_1ns_int_power         5.9475e-03
#define   ADD_1ns_switch_power      1.5593e-02
#define   ADD_1ns_leak_power        1.5927e-03
#define   ADD_1ns_area              199.583996
#define   REG_1ns_int_power         5.7056e-04
#define   REG_1ns_switch_power      7.5293e-05
#define   REG_1ns_leak_power        5.3278e-05
#define   REG_1ns_area              4.309200
#define   MUL_1ns_int_power         0.4864
#define   MUL_1ns_switch_power      0.6571
#define   MUL_1ns_leak_power        2.3933e-02
#define   MUL_1ns_area              2108.332748

/* Function units power model with TSMC 40g @ 500 MHz (2ns). */
#define   ADD_2ns_int_power         5.0460e-03
#define   ADD_2ns_switch_power      3.8079e-03
#define   ADD_2ns_leak_power        1.7153e-03
#define   ADD_2ns_area              129.275995
#define   REG_2ns_int_power         5.7059e-04
#define   REG_2ns_switch_power      7.6924e-05
#define   REG_2ns_leak_power        5.3278e-05
#define   REG_2ns_area              4.309200
#define   MUL_2ns_int_power         0.3462
#define   MUL_2ns_switch_power      0.4195
#define   MUL_2ns_leak_power        2.1248e-02
#define   MUL_2ns_area              1898.996347

/* Function units power model with TSMC 40g @ 333 MHz (3ns). */
#define   ADD_3ns_int_power         4.7682e-03
#define   ADD_3ns_switch_power      3.6085e-03
#define   ADD_3ns_leak_power        1.7153e-03
#define   ADD_3ns_area              129.275995
#define   REG_3ns_int_power         5.7059e-04
#define   REG_3ns_switch_power      7.6924e-05
#define   REG_3ns_leak_power        5.3278e-05
#define   REG_3ns_area              4.309200
#define   MUL_3ns_int_power         0.2871
#define   MUL_3ns_switch_power      0.3561
#define   MUL_3ns_leak_power        2.0740e-02
#define   MUL_3ns_area              1871.099947

/* Function units power model with TSMC 40g @ 250 MHz (4ns). */
#define   ADD_4ns_int_power         4.5124e-03
#define   ADD_4ns_switch_power      3.4215e-03
#define   ADD_4ns_leak_power        1.7152e-03
#define   ADD_4ns_area              129.275995
#define   REG_4ns_int_power         5.7059e-04
#define   REG_4ns_switch_power      7.6924e-05
#define   REG_4ns_leak_power        5.3278e-05
#define   REG_4ns_area              4.309200
#define   MUL_4ns_int_power         0.2489
#define   MUL_4ns_switch_power      0.3187
#define   MUL_4ns_leak_power        2.0738e-02
#define   MUL_4ns_area              1871.099947

/* Function units power model with TSMC 40g @ 200 MHz (5ns). */
#define   ADD_5ns_int_power         4.2887e-03
#define   ADD_5ns_switch_power      3.2556e-03
#define   ADD_5ns_leak_power        1.7153e-03
#define   ADD_5ns_area              129.275995
#define   REG_5ns_int_power         5.7059e-04
#define   REG_5ns_switch_power      7.6924e-05
#define   REG_5ns_leak_power        5.3278e-05
#define   REG_5ns_area              4.309200
#define   MUL_5ns_int_power         0.2201
#define   MUL_5ns_switch_power      0.2898
#define   MUL_5ns_leak_power        2.0739e-02
#define   MUL_5ns_area              1871.099947

/* Function units power model with TSMC 40g @ 167 MHz (6ns). */
#define   ADD_6ns_int_power         4.0978e-03
#define   ADD_6ns_switch_power      3.1096e-03
#define   ADD_6ns_leak_power        1.7153e-03
#define   ADD_6ns_area              129.275995
#define   REG_6ns_int_power         5.7059e-04
#define   REG_6ns_switch_power      7.6924e-05
#define   REG_6ns_leak_power        5.3278e-05
#define   REG_6ns_area              4.309200
#define   MUL_6ns_int_power         0.1979
#define   MUL_6ns_switch_power      0.2669
#define   MUL_6ns_leak_power        2.0739e-02
#define   MUL_6ns_area              1871.099947

void getAdderPowerArea(float cycle_time, float *internal_power,
                   float *swich_power, float *leakage_power, float *area);
void getMultiplierPowerArea(float cycle_time, float *internal_power,
                        float *switch_power, float *leakage_power, float *area);
void getRegisterPowerArea(float cycle_time, float *internal_power_per_bit,
                      float *switch_power_per_bit, float *leakage_power_per_bit,
                      float *area);
#endif
