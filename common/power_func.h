#ifndef __POWER_FUNC_H__
#define __POWER_FUNC_H__

#include <stdlib.h>
#include <iostream>

#define SINGLE_PORT_SPAD 1

/*Characterized FU latencies from FPGA virtex7 xc7v585tffg1761-2, in ns*/
/*#define   Virtex7_ADD_LATENCY       1.87*/
/*#define   Virtex7_MEMOP_LATENCY     2.39*/
/*#define   Virtex7_MUL_LATENCY       9.74*/

/* Function units power model with TSMC 40g */
/* Power in mW, energy in pJ, time in ns, area in um^2 */
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

#define ADD_0_5ns_int_power         6.7465e-02
#define ADD_0_5ns_switch_power      1.3690e-01
#define ADD_0_5ns_dynamic_power     2.0436e-01
#define ADD_0_5ns_dynamic_energy    1.0218e-01
#define ADD_0_5ns_leakage_power     2.3529e-03
#define ADD_0_5ns_area              273.293995
#define ADD_0_5ns_critical_path_delay 0.47

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

#define MUL_3STAGE_0_5ns_int_power       5.403900e+00
#define MUL_3STAGE_0_5ns_switch_power    3.693900e+00
#define MUL_3STAGE_0_5ns_dynamic_power   9.097900e+00
#define MUL_3STAGE_0_5ns_dynamic_energy  1.364685e+01
#define MUL_3STAGE_0_5ns_leakage_power   7.826750e-02
#define MUL_3STAGE_0_5ns_area             6.008386e+03

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

#define SHIFTER_1ns_int_power         5.8804e-02
#define SHIFTER_1ns_switch_power      2.4180e-01
#define SHIFTER_1ns_dynamic_power     3.0060e-01
#define SHIFTER_1ns_dynamic_energy    3.0060e-01
#define SHIFTER_1ns_leakage_power     1.2232e-03
#define SHIFTER_1ns_area              179.852394
#define SHIFTER_1ns_critical_path_delay 0.70

#define SHIFTER_2ns_int_power         2.8568e-02
#define SHIFTER_2ns_switch_power      1.1790e-01
#define SHIFTER_2ns_dynamic_power     1.4647e-01
#define SHIFTER_2ns_dynamic_energy    2.9294e-01
#define SHIFTER_2ns_leakage_power     1.2320e-03
#define SHIFTER_2ns_area              179.852394
#define SHIFTER_2ns_critical_path_delay 0.70

#define SHIFTER_3ns_int_power         2.0250e-02
#define SHIFTER_3ns_switch_power      8.1185e-02
#define SHIFTER_3ns_dynamic_power     1.0143e-01
#define SHIFTER_3ns_dynamic_energy    3.0430e-01
#define SHIFTER_3ns_leakage_power     1.2837e-03
#define SHIFTER_3ns_area              179.852394
#define SHIFTER_3ns_critical_path_delay 0.70

#define SHIFTER_4ns_int_power         1.4824e-02
#define SHIFTER_4ns_switch_power      6.0828e-02
#define SHIFTER_4ns_dynamic_power     7.5652e-02
#define SHIFTER_4ns_dynamic_energy    3.0261e-01
#define SHIFTER_4ns_leakage_power     1.2472e-03
#define SHIFTER_4ns_area              179.852394
#define SHIFTER_4ns_critical_path_delay 0.70

#define SHIFTER_5ns_int_power         1.1747e-02
#define SHIFTER_5ns_switch_power      4.8268e-02
#define SHIFTER_5ns_dynamic_power     6.0015e-02
#define SHIFTER_5ns_dynamic_energy    3.0007e-01
#define SHIFTER_5ns_leakage_power     1.2210e-03
#define SHIFTER_5ns_area              179.852394
#define SHIFTER_5ns_critical_path_delay 0.70

#define SHIFTER_6ns_int_power         9.8106e-03
#define SHIFTER_6ns_switch_power      4.0359e-02
#define SHIFTER_6ns_dynamic_power     5.0170e-02
#define SHIFTER_6ns_dynamic_energy    3.0102e-01
#define SHIFTER_6ns_leakage_power     1.2217e-03
#define SHIFTER_6ns_area              179.852394
#define SHIFTER_6ns_critical_path_delay 0.70

/* Floating Point Power/Area/Energy Models.
 * We use Synopsys DesignWare IP library to model 3-stage both single-precision
 * (SP) and double precision (DP) floating point functional units, including
 * adder/subtractor, multiplier/divider. */

#define FP_LATENCY_IN_CYCLES 3

#define FP_SP_3STAGE_ADD_1ns_int_power 1.827800e+00
#define FP_SP_3STAGE_ADD_1ns_switch_power 2.177600e+00
#define FP_SP_3STAGE_ADD_1ns_dynamic_power 4.005300e+00
#define FP_SP_3STAGE_ADD_1ns_dynamic_energy 1.201590e+01
#define FP_SP_3STAGE_ADD_1ns_leakage_power 3.826250e-02
#define FP_SP_3STAGE_ADD_1ns_area 3.141860e+03

#define FP_SP_3STAGE_ADD_2ns_int_power 5.470508e-01
#define FP_SP_3STAGE_ADD_2ns_switch_power 7.768845e-01
#define FP_SP_3STAGE_ADD_2ns_dynamic_power 1.323900e+00
#define FP_SP_3STAGE_ADD_2ns_dynamic_energy 7.943400e+00
#define FP_SP_3STAGE_ADD_2ns_leakage_power 1.548160e-02
#define FP_SP_3STAGE_ADD_2ns_area 2.166394e+03

#define FP_SP_3STAGE_ADD_3ns_int_power 3.591130e-01
#define FP_SP_3STAGE_ADD_3ns_switch_power 4.881142e-01
#define FP_SP_3STAGE_ADD_3ns_dynamic_power 8.472272e-01
#define FP_SP_3STAGE_ADD_3ns_dynamic_energy 7.625045e+00
#define FP_SP_3STAGE_ADD_3ns_leakage_power 1.453730e-02
#define FP_SP_3STAGE_ADD_3ns_area 2.134868e+03

#define FP_SP_3STAGE_ADD_4ns_int_power 2.163089e-01
#define FP_SP_3STAGE_ADD_4ns_switch_power 2.576002e-01
#define FP_SP_3STAGE_ADD_4ns_dynamic_power 4.739091e-01
#define FP_SP_3STAGE_ADD_4ns_dynamic_energy 5.686909e+00
#define FP_SP_3STAGE_ADD_4ns_leakage_power 1.133510e-02
#define FP_SP_3STAGE_ADD_4ns_area 1.525684e+03

#define FP_SP_3STAGE_ADD_5ns_int_power 1.743715e-01
#define FP_SP_3STAGE_ADD_5ns_switch_power 2.064084e-01
#define FP_SP_3STAGE_ADD_5ns_dynamic_power 3.807799e-01
#define FP_SP_3STAGE_ADD_5ns_dynamic_energy 5.711699e+00
#define FP_SP_3STAGE_ADD_5ns_leakage_power 1.095400e-02
#define FP_SP_3STAGE_ADD_5ns_area 1.486674e+03

#define FP_SP_3STAGE_ADD_6ns_int_power 1.455030e-01
#define FP_SP_3STAGE_ADD_6ns_switch_power 1.718221e-01
#define FP_SP_3STAGE_ADD_6ns_dynamic_power 3.173251e-01
#define FP_SP_3STAGE_ADD_6ns_dynamic_energy 5.711852e+00
#define FP_SP_3STAGE_ADD_6ns_leakage_power 1.091510e-02
#define FP_SP_3STAGE_ADD_6ns_area 1.486674e+03

#define FP_DP_3STAGE_ADD_0_5ns_int_power       6.496800e+00
#define FP_DP_3STAGE_ADD_0_5ns_switch_power    3.373900e+00
#define FP_DP_3STAGE_ADD_0_5ns_dynamic_power   9.870800e+00
#define FP_DP_3STAGE_ADD_0_5ns_dynamic_energy  1.480620e+01
#define FP_DP_3STAGE_ADD_0_5ns_leakage_power   1.269811e-01
#define FP_DP_3STAGE_ADD_0_5ns_area            9.262512e+03

#define FP_DP_3STAGE_ADD_1ns_int_power 4.704900e+00
#define FP_DP_3STAGE_ADD_1ns_switch_power 5.802300e+00
#define FP_DP_3STAGE_ADD_1ns_dynamic_power 1.050720e+01
#define FP_DP_3STAGE_ADD_1ns_dynamic_energy 3.152160e+01
#define FP_DP_3STAGE_ADD_1ns_leakage_power 9.596200e-02
#define FP_DP_3STAGE_ADD_1ns_area 7.121293e+03

#define FP_DP_3STAGE_ADD_2ns_int_power 1.215700e+00
#define FP_DP_3STAGE_ADD_2ns_switch_power 1.923900e+00
#define FP_DP_3STAGE_ADD_2ns_dynamic_power 3.139500e+00
#define FP_DP_3STAGE_ADD_2ns_dynamic_energy 1.883700e+01
#define FP_DP_3STAGE_ADD_2ns_leakage_power 3.244630e-02
#define FP_DP_3STAGE_ADD_2ns_area 4.414889e+03

#define FP_DP_3STAGE_ADD_3ns_int_power 7.982442e-01
#define FP_DP_3STAGE_ADD_3ns_switch_power 1.266000e+00
#define FP_DP_3STAGE_ADD_3ns_dynamic_power 2.064200e+00
#define FP_DP_3STAGE_ADD_3ns_dynamic_energy 1.857780e+01
#define FP_DP_3STAGE_ADD_3ns_leakage_power 2.945900e-02
#define FP_DP_3STAGE_ADD_3ns_area 4.308973e+03

#define FP_DP_3STAGE_ADD_4ns_int_power 5.641808e-01
#define FP_DP_3STAGE_ADD_4ns_switch_power 8.312678e-01
#define FP_DP_3STAGE_ADD_4ns_dynamic_power 1.395400e+00
#define FP_DP_3STAGE_ADD_4ns_dynamic_energy 1.674480e+01
#define FP_DP_3STAGE_ADD_4ns_leakage_power 2.518000e-02
#define FP_DP_3STAGE_ADD_4ns_area 3.685273e+03

#define FP_DP_3STAGE_ADD_5ns_int_power 4.519086e-01
#define FP_DP_3STAGE_ADD_5ns_switch_power 6.656773e-01
#define FP_DP_3STAGE_ADD_5ns_dynamic_power 1.117600e+00
#define FP_DP_3STAGE_ADD_5ns_dynamic_energy 1.676400e+01
#define FP_DP_3STAGE_ADD_5ns_leakage_power 2.525450e-02
#define FP_DP_3STAGE_ADD_5ns_area 3.686180e+03

#define FP_DP_3STAGE_ADD_6ns_int_power 3.920872e-01
#define FP_DP_3STAGE_ADD_6ns_switch_power 5.645804e-01
#define FP_DP_3STAGE_ADD_6ns_dynamic_power 9.566676e-01
#define FP_DP_3STAGE_ADD_6ns_dynamic_energy 1.722002e+01
#define FP_DP_3STAGE_ADD_6ns_leakage_power 2.756320e-02
#define FP_DP_3STAGE_ADD_6ns_area 3.690263e+03

#define FP_SP_3STAGE_MUL_1ns_int_power 3.638300e+00
#define FP_SP_3STAGE_MUL_1ns_switch_power 4.843600e+00
#define FP_SP_3STAGE_MUL_1ns_dynamic_power 8.481900e+00
#define FP_SP_3STAGE_MUL_1ns_dynamic_energy 2.544570e+01
#define FP_SP_3STAGE_MUL_1ns_leakage_power 8.633710e-02
#define FP_SP_3STAGE_MUL_1ns_area 6.460171e+03

#define FP_SP_3STAGE_MUL_2ns_int_power 1.416100e+00
#define FP_SP_3STAGE_MUL_2ns_switch_power 2.144100e+00
#define FP_SP_3STAGE_MUL_2ns_dynamic_power 3.560200e+00
#define FP_SP_3STAGE_MUL_2ns_dynamic_energy 2.136120e+01
#define FP_SP_3STAGE_MUL_2ns_leakage_power 4.280870e-02
#define FP_SP_3STAGE_MUL_2ns_area 4.433033e+03

#define FP_SP_3STAGE_MUL_3ns_int_power 8.810010e-01
#define FP_SP_3STAGE_MUL_3ns_switch_power 1.343600e+00
#define FP_SP_3STAGE_MUL_3ns_dynamic_power 2.224600e+00
#define FP_SP_3STAGE_MUL_3ns_dynamic_energy 2.002140e+01
#define FP_SP_3STAGE_MUL_3ns_leakage_power 3.558220e-02
#define FP_SP_3STAGE_MUL_3ns_area 4.103946e+03

#define FP_SP_3STAGE_MUL_4ns_int_power 5.845778e-01
#define FP_SP_3STAGE_MUL_4ns_switch_power 9.265929e-01
#define FP_SP_3STAGE_MUL_4ns_dynamic_power 1.511200e+00
#define FP_SP_3STAGE_MUL_4ns_dynamic_energy 1.813440e+01
#define FP_SP_3STAGE_MUL_4ns_leakage_power 3.244030e-02
#define FP_SP_3STAGE_MUL_4ns_area 3.760571e+03

#define FP_SP_3STAGE_MUL_5ns_int_power 4.191044e-01
#define FP_SP_3STAGE_MUL_5ns_switch_power 7.505236e-01
#define FP_SP_3STAGE_MUL_5ns_dynamic_power 1.169600e+00
#define FP_SP_3STAGE_MUL_5ns_dynamic_energy 1.754400e+01
#define FP_SP_3STAGE_MUL_5ns_leakage_power 3.037230e-02
#define FP_SP_3STAGE_MUL_5ns_area 3.534224e+03

#define FP_SP_3STAGE_MUL_6ns_int_power 3.484731e-01
#define FP_SP_3STAGE_MUL_6ns_switch_power 6.216765e-01
#define FP_SP_3STAGE_MUL_6ns_dynamic_power 9.701497e-01
#define FP_SP_3STAGE_MUL_6ns_dynamic_energy 1.746269e+01
#define FP_SP_3STAGE_MUL_6ns_leakage_power 3.024680e-02
#define FP_SP_3STAGE_MUL_6ns_area 3.522431e+03

#define FP_DP_3STAGE_MUL_0_5ns_int_power       2.006710e+01
#define FP_DP_3STAGE_MUL_0_5ns_switch_power    1.344190e+01
#define FP_DP_3STAGE_MUL_0_5ns_dynamic_power   3.350900e+01
#define FP_DP_3STAGE_MUL_0_5ns_dynamic_energy  5.026350e+01
#define FP_DP_3STAGE_MUL_0_5ns_leakage_power   2.529007e-01
#define FP_DP_3STAGE_MUL_0_5ns_area            2.059684e+04

#define FP_DP_3STAGE_MUL_1ns_int_power 1.107930e+01
#define FP_DP_3STAGE_MUL_1ns_switch_power 1.624000e+01
#define FP_DP_3STAGE_MUL_1ns_dynamic_power 2.731930e+01
#define FP_DP_3STAGE_MUL_1ns_dynamic_energy 8.195790e+01
#define FP_DP_3STAGE_MUL_1ns_leakage_power 1.991094e-01
#define FP_DP_3STAGE_MUL_1ns_area 1.604361e+04

#define FP_DP_3STAGE_MUL_2ns_int_power 4.705300e+00
#define FP_DP_3STAGE_MUL_2ns_switch_power 7.774400e+00
#define FP_DP_3STAGE_MUL_2ns_dynamic_power 1.247970e+01
#define FP_DP_3STAGE_MUL_2ns_dynamic_energy 7.487820e+01
#define FP_DP_3STAGE_MUL_2ns_leakage_power 1.403084e-01
#define FP_DP_3STAGE_MUL_2ns_area 1.301152e+04

#define FP_DP_3STAGE_MUL_3ns_int_power 2.799400e+00
#define FP_DP_3STAGE_MUL_3ns_switch_power 4.849400e+00
#define FP_DP_3STAGE_MUL_3ns_dynamic_power 7.648800e+00
#define FP_DP_3STAGE_MUL_3ns_dynamic_energy 6.883920e+01
#define FP_DP_3STAGE_MUL_3ns_leakage_power 1.103864e-01
#define FP_DP_3STAGE_MUL_3ns_area 1.176253e+04

#define FP_DP_3STAGE_MUL_4ns_int_power 2.095100e+00
#define FP_DP_3STAGE_MUL_4ns_switch_power 3.602600e+00
#define FP_DP_3STAGE_MUL_4ns_dynamic_power 5.697700e+00
#define FP_DP_3STAGE_MUL_4ns_dynamic_energy 6.837240e+01
#define FP_DP_3STAGE_MUL_4ns_leakage_power 1.089138e-01
#define FP_DP_3STAGE_MUL_4ns_area 1.170039e+04

#define FP_DP_3STAGE_MUL_5ns_int_power 1.623600e+00
#define FP_DP_3STAGE_MUL_5ns_switch_power 2.840100e+00
#define FP_DP_3STAGE_MUL_5ns_dynamic_power 4.463700e+00
#define FP_DP_3STAGE_MUL_5ns_dynamic_energy 6.695550e+01
#define FP_DP_3STAGE_MUL_5ns_leakage_power 1.010094e-01
#define FP_DP_3STAGE_MUL_5ns_area 1.135701e+04

#define FP_DP_3STAGE_MUL_6ns_int_power 1.316800e+00
#define FP_DP_3STAGE_MUL_6ns_switch_power 2.309200e+00
#define FP_DP_3STAGE_MUL_6ns_dynamic_power 3.626000e+00
#define FP_DP_3STAGE_MUL_6ns_dynamic_energy 6.526800e+01
#define FP_DP_3STAGE_MUL_6ns_leakage_power 9.714680e-02
#define FP_DP_3STAGE_MUL_6ns_area 1.106716e+04

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
void getSinglePrecisionFloatingPointAdderPowerArea(float cycle_time,
                                                   float* internal_power,
                                                   float* switch_power,
                                                   float* leakage_power,
                                                   float* area);
void getDoublePrecisionFloatingPointAdderPowerArea(float cycle_time,
                                                   float* internal_power,
                                                   float* switch_power,
                                                   float* leakage_power,
                                                   float* area);
void getSinglePrecisionFloatingPointMultiplierPowerArea(float cycle_time,
                                                        float* internal_power,
                                                        float* switch_power,
                                                        float* leakage_power,
                                                        float* area);
void getDoublePrecisionFloatingPointMultiplierPowerArea(float cycle_time,
                                                        float* internal_power,
                                                        float* switch_power,
                                                        float* leakage_power,
                                                        float* area);
#endif
