#ifndef __POWER_FUNC_H__
#define __POWER_FUNC_H__

#include <stdlib.h>
#include <iostream>

#define   SINGLE_PORT_SPAD 1

/*Characterized FU latencies from FPGA virtex7 xc7v585tffg1761-2, in ns*/
/*#define   Virtex7_ADD_LATENCY       1.87*/
/*#define   Virtex7_MEMOP_LATENCY     2.39*/
/*#define   Virtex7_MUL_LATENCY       9.74*/

/* Function units power model with TSMC 40g */
/* Power in mW, energy in pJ, time in ns. */
/* MUL     : integer MUL,DIV model*/
/* ADD     : integer ADD,SUB model*/
/* BIT     : AND,OR,XOR model*/
/* SHIFTER : Shl,LShr,AShr model*/
#define REG_1ns_int_power 5.7177e-03
#define REG_1ns_switch_power 7.6580e-04
#define REG_1ns_dynamic_power 6.4835e-03
#define REG_1ns_dynamic_energy 6.4835e-03
#define REG_1ns_leakage_power 5.3278e-05
#define REG_1ns_area 4.309200

#define REG_2ns_int_power 2.8583e-03
#define REG_2ns_switch_power 3.8773e-04
#define REG_2ns_dynamic_power 3.2461e-03
#define REG_2ns_dynamic_energy 6.4921e-03
#define REG_2ns_leakage_power 5.3278e-05
#define REG_2ns_area 4.309200

#define REG_3ns_int_power 1.9052e-03
#define REG_3ns_switch_power 2.5823e-04
#define REG_3ns_dynamic_power 2.1634e-03
#define REG_3ns_dynamic_energy 6.4903e-03
#define REG_3ns_leakage_power 5.3278e-05
#define REG_3ns_area 4.309200

#define REG_4ns_int_power 1.4281e-03
#define REG_4ns_switch_power 1.9348e-04
#define REG_4ns_dynamic_power 1.6216e-03
#define REG_4ns_dynamic_energy 6.4864e-03
#define REG_4ns_leakage_power 5.3278e-05
#define REG_4ns_area 4.309200

#define REG_5ns_int_power 1.1427e-03
#define REG_5ns_switch_power 1.5463e-04
#define REG_5ns_dynamic_power 1.2973e-03
#define REG_5ns_dynamic_energy 6.4867e-03
#define REG_5ns_leakage_power 5.3278e-05
#define REG_5ns_area 4.309200

#define REG_6ns_int_power 9.5284e-04
#define REG_6ns_switch_power 1.2911e-04
#define REG_6ns_dynamic_power 1.0820e-03
#define REG_6ns_dynamic_energy 6.4918e-03
#define REG_6ns_leakage_power 5.3278e-05
#define REG_6ns_area 4.309200

#define ADD_1ns_int_power 3.9795e-02
#define ADD_1ns_switch_power 1.1180e-01
#define ADD_1ns_dynamic_power 1.5159e-01
#define ADD_1ns_dynamic_energy 1.5159e-01
#define ADD_1ns_leakage_power 1.6209e-03
#define ADD_1ns_area 200.264396
#define ADD_1ns_critical_path_delay 0.98

#define ADD_2ns_int_power 1.7556e-02
#define ADD_2ns_switch_power 1.3333e-02
#define ADD_2ns_dynamic_power 3.0889e-02
#define ADD_2ns_dynamic_energy 6.1778e-02
#define ADD_2ns_leakage_power 1.7152e-03
#define ADD_2ns_area 129.275995
#define ADD_2ns_critical_path_delay 1.75

#define ADD_3ns_int_power 1.1719e-02
#define ADD_3ns_switch_power 8.9019e-03
#define ADD_3ns_dynamic_power 2.0621e-02
#define ADD_3ns_dynamic_energy 6.1863e-02
#define ADD_3ns_leakage_power 1.7152e-03
#define ADD_3ns_area 129.275995
#define ADD_3ns_critical_path_delay 1.75

#define ADD_4ns_int_power 8.7716e-03
#define ADD_4ns_switch_power 6.6634e-03
#define ADD_4ns_dynamic_power 1.5435e-02
#define ADD_4ns_dynamic_energy 6.1740e-02
#define ADD_4ns_leakage_power 1.7152e-03
#define ADD_4ns_area 129.275995
#define ADD_4ns_critical_path_delay 1.75

#define ADD_5ns_int_power 7.0197e-03
#define ADD_5ns_switch_power 5.3316e-03
#define ADD_5ns_dynamic_power 1.2351e-02
#define ADD_5ns_dynamic_energy 6.1756e-02
#define ADD_5ns_leakage_power 1.7152e-03
#define ADD_5ns_area 129.275995
#define ADD_5ns_critical_path_delay 1.75

#define ADD_6ns_int_power 5.8465e-03
#define ADD_6ns_switch_power 4.4399e-03
#define ADD_6ns_dynamic_power 1.0286e-02
#define ADD_6ns_dynamic_energy 6.1718e-02
#define ADD_6ns_leakage_power 1.7152e-03
#define ADD_6ns_area 129.275995
#define ADD_6ns_critical_path_delay 1.75

#define MUL_1ns_int_power 3.5876e+00
#define MUL_1ns_switch_power 5.5499e+00
#define MUL_1ns_dynamic_power 9.1375e+00
#define MUL_1ns_dynamic_energy 9.1375e+00
#define MUL_1ns_leakage_power 5.6093e-02
#define MUL_1ns_area 4575.689880
#define MUL_1ns_critical_path_delay 0.98

#define MUL_2ns_int_power 1.2599e+00
#define MUL_2ns_switch_power 2.1819e+00
#define MUL_2ns_dynamic_power 3.4418e+00
#define MUL_2ns_dynamic_energy 6.8836e+00
#define MUL_2ns_leakage_power 3.4805e-02
#define MUL_2ns_area 3701.375981
#define MUL_2ns_critical_path_delay 1.97

#define MUL_3ns_int_power 8.1100e-01
#define MUL_3ns_switch_power 1.3030e+00
#define MUL_3ns_dynamic_power 2.1140e+00
#define MUL_3ns_dynamic_energy 6.3420e+00
#define MUL_3ns_leakage_power 3.3631e-02
#define MUL_3ns_area 3583.666782
#define MUL_3ns_critical_path_delay 2.97

#define MUL_4ns_int_power 6.1970e-01
#define MUL_4ns_switch_power 9.4070e-01
#define MUL_4ns_dynamic_power 1.5604e+00
#define MUL_4ns_dynamic_energy 6.2416e+00
#define MUL_4ns_leakage_power 3.4964e-02
#define MUL_4ns_area 3328.063179
#define MUL_4ns_critical_path_delay 3.98

#define MUL_5ns_int_power 4.9470e-01
#define MUL_5ns_switch_power 7.4810e-01
#define MUL_5ns_dynamic_power 1.2428e+00
#define MUL_5ns_dynamic_energy 6.2140e+00
#define MUL_5ns_leakage_power 3.4708e-02
#define MUL_5ns_area 3310.372779
#define MUL_5ns_critical_path_delay 4.17

#define MUL_6ns_int_power 4.1250e-01
#define MUL_6ns_switch_power 6.2410e-01
#define MUL_6ns_dynamic_power 1.0366e+00
#define MUL_6ns_dynamic_energy 6.2196e+00
#define MUL_6ns_leakage_power 3.4708e-02
#define MUL_6ns_area 3310.372779
#define MUL_6ns_critical_path_delay 4.17

#define BIT_1ns_int_power 7.2807e-03
#define BIT_1ns_switch_power 5.7277e-03
#define BIT_1ns_dynamic_power 1.3008e-02
#define BIT_1ns_dynamic_energy 1.3008e-02
#define BIT_1ns_leakage_power 4.4030e-04
#define BIT_1ns_area 36.287998
#define BIT_1ns_critical_path_delay 0.06

#define BIT_2ns_int_power 3.6367e-03
#define BIT_2ns_switch_power 2.8610e-03
#define BIT_2ns_dynamic_power 6.4977e-03
#define BIT_2ns_dynamic_energy 1.2995e-02
#define BIT_2ns_leakage_power 4.4030e-04
#define BIT_2ns_area 36.287998
#define BIT_2ns_critical_path_delay 0.06

#define BIT_3ns_int_power 2.4221e-03
#define BIT_3ns_switch_power 1.9054e-03
#define BIT_3ns_dynamic_power 4.3275e-03
#define BIT_3ns_dynamic_energy 1.2982e-02
#define BIT_3ns_leakage_power 4.4030e-04
#define BIT_3ns_area 36.287998
#define BIT_3ns_critical_path_delay 0.06

#define BIT_4ns_int_power 1.8147e-03
#define BIT_4ns_switch_power 1.4276e-03
#define BIT_4ns_dynamic_power 3.2423e-03
#define BIT_4ns_dynamic_energy 1.2969e-02
#define BIT_4ns_leakage_power 4.4030e-04
#define BIT_4ns_area 36.287998
#define BIT_4ns_critical_path_delay 0.06

#define BIT_5ns_int_power 1.4503e-03
#define BIT_5ns_switch_power 1.1410e-03
#define BIT_5ns_dynamic_power 2.5913e-03
#define BIT_5ns_dynamic_energy 1.2957e-02
#define BIT_5ns_leakage_power 4.4030e-04
#define BIT_5ns_area 36.287998
#define BIT_5ns_critical_path_delay 0.06

#define BIT_6ns_int_power 1.2110e-03
#define BIT_6ns_switch_power 9.5271e-04
#define BIT_6ns_dynamic_power 2.1637e-03
#define BIT_6ns_dynamic_energy 1.2982e-02
#define BIT_6ns_leakage_power 4.4030e-04
#define BIT_6ns_area 36.287998
#define BIT_6ns_critical_path_delay 0.06

#define SHIFTER_1ns_int_power 2.4600e-01
#define SHIFTER_1ns_switch_power 3.2810e-01
#define SHIFTER_1ns_dynamic_power 5.7410e-01
#define SHIFTER_1ns_dynamic_energy 5.7410e-01
#define SHIFTER_1ns_leakage_power 2.9164e-03
#define SHIFTER_1ns_area 374.446796
#define SHIFTER_1ns_critical_path_delay 0.70

#define SHIFTER_2ns_int_power 1.2460e-01
#define SHIFTER_2ns_switch_power 1.6610e-01
#define SHIFTER_2ns_dynamic_power 2.9070e-01
#define SHIFTER_2ns_dynamic_energy 5.8140e-01
#define SHIFTER_2ns_leakage_power 2.9207e-03
#define SHIFTER_2ns_area 374.446796
#define SHIFTER_2ns_critical_path_delay 0.70

#define SHIFTER_3ns_int_power 8.3166e-02
#define SHIFTER_3ns_switch_power 1.1080e-01
#define SHIFTER_3ns_dynamic_power 1.9397e-01
#define SHIFTER_3ns_dynamic_energy 5.8190e-01
#define SHIFTER_3ns_leakage_power 2.9273e-03
#define SHIFTER_3ns_area 374.446796
#define SHIFTER_3ns_critical_path_delay 0.70

#define SHIFTER_4ns_int_power 6.2652e-02
#define SHIFTER_4ns_switch_power 8.3439e-02
#define SHIFTER_4ns_dynamic_power 1.4609e-01
#define SHIFTER_4ns_dynamic_energy 5.8436e-01
#define SHIFTER_4ns_leakage_power 2.9286e-03
#define SHIFTER_4ns_area 374.446796
#define SHIFTER_4ns_critical_path_delay 0.70

#define SHIFTER_5ns_int_power 5.0285e-02
#define SHIFTER_5ns_switch_power 6.6852e-02
#define SHIFTER_5ns_dynamic_power 1.1714e-01
#define SHIFTER_5ns_dynamic_energy 5.8569e-01
#define SHIFTER_5ns_leakage_power 2.9315e-03
#define SHIFTER_5ns_area 374.446796
#define SHIFTER_5ns_critical_path_delay 0.70

#define SHIFTER_6ns_int_power 4.1957e-02
#define SHIFTER_6ns_switch_power 5.5785e-02
#define SHIFTER_6ns_dynamic_power 9.7742e-01
#define SHIFTER_6ns_dynamic_energy 5.8645e-01
#define SHIFTER_6ns_leakage_power 2.9317e-03
#define SHIFTER_6ns_area 374.446796
#define SHIFTER_6ns_critical_path_delay 0.70

void getRegisterPowerArea(float cycle_time,
                          float* internal_power_per_bit,
                          float* switch_power_per_bit,
                          float* leakage_power_per_bit,
                          float* area);
void getAdderPowerArea(float cycle_time,
                       float* internal_power,
                       float* swich_power,
                       float* leakage_power,
                       float* area);
void getMultiplierPowerArea(float cycle_time,
                            float* internal_power,
                            float* switch_power,
                            float* leakage_power,
                            float* area);
void getBitPowerArea(float cycle_time,
                     float* internal_power,
                     float* swich_power,
                     float* leakage_power,
                     float* area);
void getShifterPowerArea(float cycle_time,
                         float* internal_power,
                         float* swich_power,
                         float* leakage_power,
                         float* area);
#endif
