#ifndef __POWER_FUNC_H__

#include <stdlib.h>
#include <iostream>
#include "cacti-p/io.h"
#include "cacti-p/cacti_interface.h"

#define SINGLE_PORT_SPAD 1

/*Characterized FU latencies from FPGA virtex7 xc7v585tffg1761-2, in ns*/
/*#define   Virtex7_ADD_LATENCY       1.87*/
/*#define   Virtex7_MEMOP_LATENCY     2.39*/
/*#define   Virtex7_MUL_LATENCY       9.74*/

/* Function units power model normalized to a commercial 40g */
/* Power in mW, energy in pJ, time in ns, area in um^2 */
/* MUL     : integer MUL,DIV model*/
/* ADD     : integer ADD,SUB model*/
/* BIT     : AND,OR,XOR model*/
/* SHIFTER : Shl,LShr,AShr model*/

#define REG_1ns_int_power                             7.936518e-03
#define REG_1ns_switch_power                          1.062977e-03
#define REG_1ns_dynamic_power                         8.999495e-03
#define REG_1ns_dynamic_energy                        8.999495e-03
#define REG_1ns_leakage_power                         7.395312e-05
#define REG_1ns_area                                  5.981433e+00

#define REG_2ns_int_power                             3.967495e-03
#define REG_2ns_switch_power                          5.381930e-04
#define REG_2ns_dynamic_power                         4.505785e-03
#define REG_2ns_dynamic_energy                        9.011432e-03
#define REG_2ns_leakage_power                         7.395312e-05
#define REG_2ns_area                                  5.981433e+00

#define REG_3ns_int_power                             2.644534e-03
#define REG_3ns_switch_power                          3.584390e-04
#define REG_3ns_dynamic_power                         3.002932e-03
#define REG_3ns_dynamic_energy                        9.008934e-03
#define REG_3ns_leakage_power                         7.395312e-05
#define REG_3ns_area                                  5.981433e+00

#define REG_4ns_int_power                             1.982290e-03
#define REG_4ns_switch_power                          2.685621e-04
#define REG_4ns_dynamic_power                         2.250880e-03
#define REG_4ns_dynamic_energy                        9.003520e-03
#define REG_4ns_leakage_power                         7.395312e-05
#define REG_4ns_area                                  5.981433e+00

#define REG_5ns_int_power                             1.586138e-03
#define REG_5ns_switch_power                          2.146359e-04
#define REG_5ns_dynamic_power                         1.800732e-03
#define REG_5ns_dynamic_energy                        9.003937e-03
#define REG_5ns_leakage_power                         7.395312e-05
#define REG_5ns_area                                  5.981433e+00

#define REG_6ns_int_power                             1.322600e-03
#define REG_6ns_switch_power                          1.792126e-04
#define REG_6ns_dynamic_power                         1.501882e-03
#define REG_6ns_dynamic_energy                        9.011016e-03
#define REG_6ns_leakage_power                         7.395312e-05
#define REG_6ns_area                                  5.981433e+00

#define REG_10ns_int_power                            5.535310e-04
#define REG_10ns_switch_power                         0.000000e+00
#define REG_10ns_dynamic_power                        5.535310e-04
#define REG_10ns_dynamic_energy                       5.535310e-03
#define REG_10ns_leakage_power                        7.358945e-05
#define REG_10ns_area                                 5.981433e+00

#define ADD_0_5ns_int_power                           9.364555e-02
#define ADD_0_5ns_switch_power                        1.900256e-01
#define ADD_0_5ns_dynamic_power                       2.836642e-01
#define ADD_0_5ns_dynamic_energy                      1.418321e-01
#define ADD_0_5ns_leakage_power                       3.265969e-03
#define ADD_0_5ns_area                                3.793488e+02
#define ADD_0_5ns_critical_path_delay                 0.47

#define ADD_1ns_int_power                             5.523790e-02
#define ADD_1ns_switch_power                          1.551852e-01
#define ADD_1ns_dynamic_power                         2.104162e-01
#define ADD_1ns_dynamic_energy                        2.104162e-01
#define ADD_1ns_leakage_power                         2.249908e-03
#define ADD_1ns_area                                  2.779792e+02
#define ADD_1ns_critical_path_delay                   0.98

#define ADD_2ns_int_power                             2.436880e-02
#define ADD_2ns_switch_power                          1.850702e-02
#define ADD_2ns_dynamic_power                         4.287582e-02
#define ADD_2ns_dynamic_energy                        8.575164e-02
#define ADD_2ns_leakage_power                         2.380803e-03
#define ADD_2ns_area                                  1.794430e+02
#define ADD_2ns_critical_path_delay                   1.75

#define ADD_3ns_int_power                             1.626669e-02
#define ADD_3ns_switch_power                          1.235638e-02
#define ADD_3ns_dynamic_power                         2.862321e-02
#define ADD_3ns_dynamic_energy                        8.586963e-02
#define ADD_3ns_leakage_power                         2.380803e-03
#define ADD_3ns_area                                  1.794430e+02
#define ADD_3ns_critical_path_delay                   1.75

#define ADD_4ns_int_power                             1.217552e-02
#define ADD_4ns_switch_power                          9.249207e-03
#define ADD_4ns_dynamic_power                         2.142472e-02
#define ADD_4ns_dynamic_energy                        8.569890e-02
#define ADD_4ns_leakage_power                         2.380803e-03
#define ADD_4ns_area                                  1.794430e+02
#define ADD_4ns_critical_path_delay                   1.75

#define ADD_5ns_int_power                             9.743773e-03
#define ADD_5ns_switch_power                          7.400587e-03
#define ADD_5ns_dynamic_power                         1.714394e-02
#define ADD_5ns_dynamic_energy                        8.572111e-02
#define ADD_5ns_leakage_power                         2.380803e-03
#define ADD_5ns_area                                  1.794430e+02
#define ADD_5ns_critical_path_delay                   1.75

#define ADD_6ns_int_power                             8.115300e-03
#define ADD_6ns_switch_power                          6.162853e-03
#define ADD_6ns_dynamic_power                         1.427760e-02
#define ADD_6ns_dynamic_energy                        8.566836e-02
#define ADD_6ns_leakage_power                         2.380803e-03
#define ADD_6ns_area                                  1.794430e+02
#define ADD_6ns_critical_path_delay                   1.75

#define ADD_10ns_int_power                            4.832257e-03
#define ADD_10ns_switch_power                         3.701820e-03
#define ADD_10ns_dynamic_power                        8.534078e-03
#define ADD_10ns_dynamic_energy                       8.534078e-02
#define ADD_10ns_leakage_power                        2.380803e-03
#define ADD_10ns_area                                 1.794430e+02
#define ADD_10ns_critical_path_delay                  1.75

#define MUL_3STAGE_0_5ns_int_power                    7.500944e+00
#define MUL_3STAGE_0_5ns_switch_power                 5.127359e+00
#define MUL_3STAGE_0_5ns_dynamic_power                1.262844e+01
#define MUL_3STAGE_0_5ns_dynamic_energy               1.894266e+01
#define MUL_3STAGE_0_5ns_leakage_power                1.086401e-01
#define MUL_3STAGE_0_5ns_area                         8.340007e+03

#define MUL_1ns_int_power                             4.979808e+00
#define MUL_1ns_switch_power                          7.703601e+00
#define MUL_1ns_dynamic_power                         1.268341e+01
#define MUL_1ns_dynamic_energy                        1.268341e+01
#define MUL_1ns_leakage_power                         7.786052e-02
#define MUL_1ns_area                                  6.351338e+03
#define MUL_1ns_critical_path_delay                   0.98

#define MUL_2ns_int_power                             1.748818e+00
#define MUL_2ns_switch_power                          3.028611e+00
#define MUL_2ns_dynamic_power                         4.777429e+00
#define MUL_2ns_dynamic_energy                        9.554858e+00
#define MUL_2ns_leakage_power                         4.831147e-02
#define MUL_2ns_area                                  5.137736e+03
#define MUL_2ns_critical_path_delay                   1.97

#define MUL_3ns_int_power                             1.125718e+00
#define MUL_3ns_switch_power                          1.808644e+00
#define MUL_3ns_dynamic_power                         2.934361e+00
#define MUL_3ns_dynamic_energy                        8.803084e+00
#define MUL_3ns_leakage_power                         4.668189e-02
#define MUL_3ns_area                                  4.974349e+03
#define MUL_3ns_critical_path_delay                   2.97

#define MUL_4ns_int_power                             8.601815e-01
#define MUL_4ns_switch_power                          1.305749e+00
#define MUL_4ns_dynamic_power                         2.165931e+00
#define MUL_4ns_dynamic_energy                        8.663723e+00
#define MUL_4ns_leakage_power                         4.853217e-02
#define MUL_4ns_area                                  4.619555e+03
#define MUL_4ns_critical_path_delay                   3.98

#define MUL_5ns_int_power                             6.866739e-01
#define MUL_5ns_switch_power                          1.038409e+00
#define MUL_5ns_dynamic_power                         1.725082e+00
#define MUL_5ns_dynamic_energy                        8.625412e+00
#define MUL_5ns_leakage_power                         4.817683e-02
#define MUL_5ns_area                                  4.595000e+03
#define MUL_5ns_critical_path_delay                   4.17

#define MUL_6ns_int_power                             5.725752e-01
#define MUL_6ns_switch_power                          8.662890e-01
#define MUL_6ns_dynamic_power                         1.438864e+00
#define MUL_6ns_dynamic_energy                        8.633185e+00
#define MUL_6ns_leakage_power                         4.817683e-02
#define MUL_6ns_area                                  4.595000e+03
#define MUL_6ns_critical_path_delay                   4.17

#define MUL_10ns_int_power                            3.459049e-01
#define MUL_10ns_switch_power                         5.337095e-01
#define MUL_10ns_dynamic_power                        8.796144e-01
#define MUL_10ns_dynamic_energy                       8.796144e+00
#define MUL_10ns_leakage_power                        4.817683e-02
#define MUL_10ns_area                                 4.595000e+03
#define MUL_10ns_critical_path_delay                  4.90

#define BIT_1ns_int_power                             1.010606e-02
#define BIT_1ns_switch_power                          7.950398e-03
#define BIT_1ns_dynamic_power                         1.805590e-02
#define BIT_1ns_dynamic_energy                        1.805590e-02
#define BIT_1ns_leakage_power                         6.111633e-04
#define BIT_1ns_area                                  5.036996e+01
#define BIT_1ns_critical_path_delay                   0.06

#define BIT_2ns_int_power                             5.047962e-03
#define BIT_2ns_switch_power                          3.971243e-03
#define BIT_2ns_dynamic_power                         9.019205e-03
#define BIT_2ns_dynamic_energy                        1.803786e-02
#define BIT_2ns_leakage_power                         6.111633e-04
#define BIT_2ns_area                                  5.036996e+01
#define BIT_2ns_critical_path_delay                   0.06

#define BIT_3ns_int_power                             3.362023e-03
#define BIT_3ns_switch_power                          2.644812e-03
#define BIT_3ns_dynamic_power                         6.006835e-03
#define BIT_3ns_dynamic_energy                        1.801981e-02
#define BIT_3ns_leakage_power                         6.111633e-04
#define BIT_3ns_area                                  5.036996e+01
#define BIT_3ns_critical_path_delay                   0.06

#define BIT_4ns_int_power                             2.518915e-03
#define BIT_4ns_switch_power                          1.981596e-03
#define BIT_4ns_dynamic_power                         4.500511e-03
#define BIT_4ns_dynamic_energy                        1.800177e-02
#define BIT_4ns_leakage_power                         6.111633e-04
#define BIT_4ns_area                                  5.036996e+01
#define BIT_4ns_critical_path_delay                   0.06

#define BIT_5ns_int_power                             2.013105e-03
#define BIT_5ns_switch_power                          1.583778e-03
#define BIT_5ns_dynamic_power                         3.596883e-03
#define BIT_5ns_dynamic_energy                        1.798511e-02
#define BIT_5ns_leakage_power                         6.111633e-04
#define BIT_5ns_area                                  5.036996e+01
#define BIT_5ns_critical_path_delay                   0.06

#define BIT_6ns_int_power                             1.680942e-03
#define BIT_6ns_switch_power                          1.322420e-03
#define BIT_6ns_dynamic_power                         3.003348e-03
#define BIT_6ns_dynamic_energy                        1.801981e-02
#define BIT_6ns_leakage_power                         6.111633e-04
#define BIT_6ns_area                                  5.036996e+01
#define BIT_6ns_critical_path_delay                   0.06

#define BIT_10ns_int_power                            1.011508e-03
#define BIT_10ns_switch_power                         7.957616e-04
#define BIT_10ns_dynamic_power                        1.807256e-03
#define BIT_10ns_dynamic_energy                       1.807256e-02
#define BIT_10ns_leakage_power                        6.111633e-04
#define BIT_10ns_area                                 5.036996e+01
#define BIT_10ns_critical_path_delay                  0.06

#define SHIFTER_1ns_int_power                         8.162355e-02
#define SHIFTER_1ns_switch_power                      3.356332e-01
#define SHIFTER_1ns_dynamic_power                     4.172512e-01
#define SHIFTER_1ns_dynamic_energy                    4.172512e-01
#define SHIFTER_1ns_leakage_power                     1.697876e-03
#define SHIFTER_1ns_area                              2.496461e+02
#define SHIFTER_1ns_critical_path_delay               0.70

#define SHIFTER_2ns_int_power                         3.965413e-02
#define SHIFTER_2ns_switch_power                      1.636524e-01
#define SHIFTER_2ns_dynamic_power                     2.033093e-01
#define SHIFTER_2ns_dynamic_energy                    4.066186e-01
#define SHIFTER_2ns_leakage_power                     1.710091e-03
#define SHIFTER_2ns_area                              2.496461e+02
#define SHIFTER_2ns_critical_path_delay               0.70

#define SHIFTER_3ns_int_power                         2.810824e-02
#define SHIFTER_3ns_switch_power                      1.126897e-01
#define SHIFTER_3ns_dynamic_power                     1.407910e-01
#define SHIFTER_3ns_dynamic_energy                    4.223870e-01
#define SHIFTER_3ns_leakage_power                     1.781854e-03
#define SHIFTER_3ns_area                              2.496461e+02
#define SHIFTER_3ns_critical_path_delay               0.70

#define SHIFTER_4ns_int_power                         2.057662e-02
#define SHIFTER_4ns_switch_power                      8.443299e-02
#define SHIFTER_4ns_dynamic_power                     1.050096e-01
#define SHIFTER_4ns_dynamic_energy                    4.200412e-01
#define SHIFTER_4ns_leakage_power                     1.731190e-03
#define SHIFTER_4ns_area                              2.496461e+02
#define SHIFTER_4ns_critical_path_delay               0.70

#define SHIFTER_5ns_int_power                         1.630555e-02
#define SHIFTER_5ns_switch_power                      6.699894e-02
#define SHIFTER_5ns_dynamic_power                     8.330449e-02
#define SHIFTER_5ns_dynamic_energy                    4.165155e-01
#define SHIFTER_5ns_leakage_power                     1.694823e-03
#define SHIFTER_5ns_area                              2.496461e+02
#define SHIFTER_5ns_critical_path_delay               0.70

#define SHIFTER_6ns_int_power                         1.361771e-02
#define SHIFTER_6ns_switch_power                      5.602076e-02
#define SHIFTER_6ns_dynamic_power                     6.963903e-02
#define SHIFTER_6ns_dynamic_energy                    4.178342e-01
#define SHIFTER_6ns_leakage_power                     1.695794e-03
#define SHIFTER_6ns_area                              2.496461e+02
#define SHIFTER_6ns_critical_path_delay               0.70

#define SHIFTER_10ns_int_power                        8.120019e-03
#define SHIFTER_10ns_switch_power                     3.338565e-02
#define SHIFTER_10ns_dynamic_power                    4.150581e-02
#define SHIFTER_10ns_dynamic_energy                   4.150581e-01
#define SHIFTER_10ns_leakage_power                    1.695100e-03
#define SHIFTER_10ns_area                             2.496461e+02
#define SHIFTER_10ns_critical_path_delay              0.70

/* Floating Point Power/Area/Energy Models.
 * We use Synopsys DesignWare IP library to model 3-stage both single-precision
 * (SP) and double precision (DP) floating point functional units, including
 * adder/subtractor, multiplier/divider. */

#define FP_MUL_LATENCY_IN_CYCLES 4
#define FP_ADD_LATENCY_IN_CYCLES 5
#define FP_DIV_LATENCY_IN_CYCLES 16
#define TRIG_SINE_LATENCY_IN_CYCLES 3

#define FP_SP_3STAGE_ADD_1ns_int_power                2.537098e+00
#define FP_SP_3STAGE_ADD_1ns_switch_power             3.022642e+00
#define FP_SP_3STAGE_ADD_1ns_dynamic_power            5.559602e+00
#define FP_SP_3STAGE_ADD_1ns_dynamic_energy           1.667880e+01
#define FP_SP_3STAGE_ADD_1ns_leakage_power            5.311069e-02
#define FP_SP_3STAGE_ADD_1ns_area                     4.361094e+03

#define FP_SP_3STAGE_ADD_2ns_int_power                7.593400e-01
#define FP_SP_3STAGE_ADD_2ns_switch_power             1.078363e+00
#define FP_SP_3STAGE_ADD_2ns_dynamic_power            1.837654e+00
#define FP_SP_3STAGE_ADD_2ns_dynamic_energy           1.102593e+01
#define FP_SP_3STAGE_ADD_2ns_leakage_power            2.148941e-02
#define FP_SP_3STAGE_ADD_2ns_area                     3.007087e+03

#define FP_SP_3STAGE_ADD_3ns_int_power                4.984708e-01
#define FP_SP_3STAGE_ADD_3ns_switch_power             6.775324e-01
#define FP_SP_3STAGE_ADD_3ns_dynamic_power            1.176003e+00
#define FP_SP_3STAGE_ADD_3ns_dynamic_energy           1.058403e+01
#define FP_SP_3STAGE_ADD_3ns_leakage_power            2.017866e-02
#define FP_SP_3STAGE_ADD_3ns_area                     2.963327e+03

#define FP_SP_3STAGE_ADD_4ns_int_power                3.002500e-01
#define FP_SP_3STAGE_ADD_4ns_switch_power             3.575648e-01
#define FP_SP_3STAGE_ADD_4ns_dynamic_power            6.578148e-01
#define FP_SP_3STAGE_ADD_4ns_dynamic_energy           7.893778e+00
#define FP_SP_3STAGE_ADD_4ns_leakage_power            1.573381e-02
#define FP_SP_3STAGE_ADD_4ns_area                     2.117743e+03

#define FP_SP_3STAGE_ADD_5ns_int_power                2.420383e-01
#define FP_SP_3STAGE_ADD_5ns_switch_power             2.865075e-01
#define FP_SP_3STAGE_ADD_5ns_dynamic_power            5.285458e-01
#define FP_SP_3STAGE_ADD_5ns_dynamic_energy           7.928188e+00
#define FP_SP_3STAGE_ADD_5ns_leakage_power            1.520482e-02
#define FP_SP_3STAGE_ADD_5ns_area                     2.063594e+03

#define FP_SP_3STAGE_ADD_6ns_int_power                2.019671e-01
#define FP_SP_3STAGE_ADD_6ns_switch_power             2.384996e-01
#define FP_SP_3STAGE_ADD_6ns_dynamic_power            4.404667e-01
#define FP_SP_3STAGE_ADD_6ns_dynamic_energy           7.928400e+00
#define FP_SP_3STAGE_ADD_6ns_leakage_power            1.515083e-02
#define FP_SP_3STAGE_ADD_6ns_area                     2.063594e+03

#define FP_SP_3STAGE_ADD_10ns_int_power               1.171926e+00
#define FP_SP_3STAGE_ADD_10ns_switch_power            1.395194e-01
#define FP_SP_3STAGE_ADD_10ns_dynamic_power           2.567121e-01
#define FP_SP_3STAGE_ADD_10ns_dynamic_energy          7.701362e+00
#define FP_SP_3STAGE_ADD_10ns_leakage_power           1.525465e-02
#define FP_SP_3STAGE_ADD_10ns_area                    2.063594e+03

#define FP_DP_3STAGE_ADD_0_5ns_int_power              9.017956e+00
#define FP_DP_3STAGE_ADD_0_5ns_switch_power           4.683180e+00
#define FP_DP_3STAGE_ADD_0_5ns_dynamic_power          1.370127e+01
#define FP_DP_3STAGE_ADD_0_5ns_dynamic_energy         2.055191e+01
#define FP_DP_3STAGE_ADD_0_5ns_leakage_power          1.762575e-01
#define FP_DP_3STAGE_ADD_0_5ns_area                   1.285693e+04

#define FP_DP_3STAGE_ADD_1ns_int_power                6.530689e+00
#define FP_DP_3STAGE_ADD_1ns_switch_power             8.053947e+00
#define FP_DP_3STAGE_ADD_1ns_dynamic_power            1.458464e+01
#define FP_DP_3STAGE_ADD_1ns_dynamic_energy           4.375391e+01
#define FP_DP_3STAGE_ADD_1ns_leakage_power            1.332011e-01
#define FP_DP_3STAGE_ADD_1ns_area                     9.884790e+03

#define FP_DP_3STAGE_ADD_2ns_int_power                1.687466e+00
#define FP_DP_3STAGE_ADD_2ns_switch_power             2.670491e+00
#define FP_DP_3STAGE_ADD_2ns_dynamic_power            4.357818e+00
#define FP_DP_3STAGE_ADD_2ns_dynamic_energy           2.614691e+01
#define FP_DP_3STAGE_ADD_2ns_leakage_power            4.503745e-02
#define FP_DP_3STAGE_ADD_2ns_area                     6.128136e+03

#define FP_DP_3STAGE_ADD_3ns_int_power                1.108012e+00
#define FP_DP_3STAGE_ADD_3ns_switch_power             1.757285e+00
#define FP_DP_3STAGE_ADD_3ns_dynamic_power            2.865236e+00
#define FP_DP_3STAGE_ADD_3ns_dynamic_energy           2.578712e+01
#define FP_DP_3STAGE_ADD_3ns_leakage_power            4.089089e-02
#define FP_DP_3STAGE_ADD_3ns_area                     5.981118e+03

#define FP_DP_3STAGE_ADD_4ns_int_power                7.831175e-01
#define FP_DP_3STAGE_ADD_4ns_switch_power             1.153851e+00
#define FP_DP_3STAGE_ADD_4ns_dynamic_power            1.936901e+00
#define FP_DP_3STAGE_ADD_4ns_dynamic_energy           2.324281e+01
#define FP_DP_3STAGE_ADD_4ns_leakage_power            3.495138e-02
#define FP_DP_3STAGE_ADD_4ns_area                     5.115384e+03

#define FP_DP_3STAGE_ADD_5ns_int_power                6.272768e-01
#define FP_DP_3STAGE_ADD_5ns_switch_power             9.240008e-01
#define FP_DP_3STAGE_ADD_5ns_dynamic_power            1.551297e+00
#define FP_DP_3STAGE_ADD_5ns_dynamic_energy           2.326946e+01
#define FP_DP_3STAGE_ADD_5ns_leakage_power            3.505479e-02
#define FP_DP_3STAGE_ADD_5ns_area                     5.116643e+03

#define FP_DP_3STAGE_ADD_6ns_int_power                5.442410e-01
#define FP_DP_3STAGE_ADD_6ns_switch_power             7.836721e-01
#define FP_DP_3STAGE_ADD_6ns_dynamic_power            1.327913e+00
#define FP_DP_3STAGE_ADD_6ns_dynamic_energy           2.390244e+01
#define FP_DP_3STAGE_ADD_6ns_leakage_power            3.825941e-02
#define FP_DP_3STAGE_ADD_6ns_area                     5.122311e+03

#define FP_DP_3STAGE_ADD_10ns_int_power               2.413056e-01
#define FP_DP_3STAGE_ADD_10ns_switch_power            3.513326e-01
#define FP_DP_3STAGE_ADD_10ns_dynamic_power           5.926381e-01
#define FP_DP_3STAGE_ADD_10ns_dynamic_energy          1.777915e+01
#define FP_DP_3STAGE_ADD_10ns_leakage_power           3.017909e-02
#define FP_DP_3STAGE_ADD_10ns_area                    3.989615e+03

#define FP_SP_3STAGE_MUL_1ns_int_power                5.050183e+00
#define FP_SP_3STAGE_MUL_1ns_switch_power             6.723213e+00
#define FP_SP_3STAGE_MUL_1ns_dynamic_power            1.177340e+01
#define FP_SP_3STAGE_MUL_1ns_dynamic_energy           3.532019e+01
#define FP_SP_3STAGE_MUL_1ns_leakage_power            1.198412e-01
#define FP_SP_3STAGE_MUL_1ns_area                     8.967113e+03

#define FP_SP_3STAGE_MUL_2ns_int_power                1.965633e+00
#define FP_SP_3STAGE_MUL_2ns_switch_power             2.976142e+00
#define FP_SP_3STAGE_MUL_2ns_dynamic_power            4.941775e+00
#define FP_SP_3STAGE_MUL_2ns_dynamic_energy           2.965065e+01
#define FP_SP_3STAGE_MUL_2ns_leakage_power            5.942110e-02
#define FP_SP_3STAGE_MUL_2ns_area                     6.153321e+03

#define FP_SP_3STAGE_MUL_3ns_int_power                1.222883e+00
#define FP_SP_3STAGE_MUL_3ns_switch_power             1.864999e+00
#define FP_SP_3STAGE_MUL_3ns_dynamic_power            3.087881e+00
#define FP_SP_3STAGE_MUL_3ns_dynamic_energy           2.779093e+01
#define FP_SP_3STAGE_MUL_3ns_leakage_power            4.939027e-02
#define FP_SP_3STAGE_MUL_3ns_area                     5.696528e+03

#define FP_SP_3STAGE_MUL_4ns_int_power                8.114298e-01
#define FP_SP_3STAGE_MUL_4ns_switch_power             1.286168e+00
#define FP_SP_3STAGE_MUL_4ns_dynamic_power            2.097638e+00
#define FP_SP_3STAGE_MUL_4ns_dynamic_energy           2.517166e+01
#define FP_SP_3STAGE_MUL_4ns_leakage_power            4.502912e-02
#define FP_SP_3STAGE_MUL_4ns_area                     5.219903e+03

#define FP_SP_3STAGE_MUL_5ns_int_power                5.817426e-01
#define FP_SP_3STAGE_MUL_5ns_switch_power             1.041773e+00
#define FP_SP_3STAGE_MUL_5ns_dynamic_power            1.623476e+00
#define FP_SP_3STAGE_MUL_5ns_dynamic_energy           2.435215e+01
#define FP_SP_3STAGE_MUL_5ns_leakage_power            4.215861e-02
#define FP_SP_3STAGE_MUL_5ns_area                     4.905719e+03

#define FP_SP_3STAGE_MUL_6ns_int_power                4.837020e-01
#define FP_SP_3STAGE_MUL_6ns_switch_power             8.629250e-01
#define FP_SP_3STAGE_MUL_6ns_dynamic_power            1.346627e+00
#define FP_SP_3STAGE_MUL_6ns_dynamic_energy           2.423928e+01
#define FP_SP_3STAGE_MUL_6ns_leakage_power            4.198441e-02
#define FP_SP_3STAGE_MUL_6ns_area                     4.889350e+03

#define FP_SP_3STAGE_MUL_10ns_int_power               2.908399e-01
#define FP_SP_3STAGE_MUL_10ns_switch_power            5.956748e-01
#define FP_SP_3STAGE_MUL_10ns_dynamic_power           8.865146e-01
#define FP_SP_3STAGE_MUL_10ns_dynamic_energy          2.659543e+01
#define FP_SP_3STAGE_MUL_10ns_leakage_power           4.476567e-02
#define FP_SP_3STAGE_MUL_10ns_area                    5.421698e+03

#define FP_DP_3STAGE_MUL_0_5ns_int_power              2.785436e+01
#define FP_DP_3STAGE_MUL_0_5ns_switch_power           1.865818e+01
#define FP_DP_3STAGE_MUL_0_5ns_dynamic_power          4.651254e+01
#define FP_DP_3STAGE_MUL_0_5ns_dynamic_energy         6.976881e+01
#define FP_DP_3STAGE_MUL_0_5ns_leakage_power          3.510416e-01
#define FP_DP_3STAGE_MUL_0_5ns_area                   2.858967e+04

#define FP_DP_3STAGE_MUL_1ns_int_power                1.537875e+01
#define FP_DP_3STAGE_MUL_1ns_switch_power             2.254211e+01
#define FP_DP_3STAGE_MUL_1ns_dynamic_power            3.792086e+01
#define FP_DP_3STAGE_MUL_1ns_dynamic_energy           1.137626e+02
#define FP_DP_3STAGE_MUL_1ns_leakage_power            2.763760e-01
#define FP_DP_3STAGE_MUL_1ns_area                     2.226951e+04

#define FP_DP_3STAGE_MUL_2ns_int_power                6.531244e+00
#define FP_DP_3STAGE_MUL_2ns_switch_power             1.079134e+01
#define FP_DP_3STAGE_MUL_2ns_dynamic_power            1.732259e+01
#define FP_DP_3STAGE_MUL_2ns_dynamic_energy           1.039355e+02
#define FP_DP_3STAGE_MUL_2ns_leakage_power            1.947566e-01
#define FP_DP_3STAGE_MUL_2ns_area                     1.806079e+04

#define FP_DP_3STAGE_MUL_3ns_int_power                3.885739e+00
#define FP_DP_3STAGE_MUL_3ns_switch_power             6.731264e+00
#define FP_DP_3STAGE_MUL_3ns_dynamic_power            1.061700e+01
#define FP_DP_3STAGE_MUL_3ns_dynamic_energy           9.555302e+01
#define FP_DP_3STAGE_MUL_3ns_leakage_power            1.532231e-01
#define FP_DP_3STAGE_MUL_3ns_area                     1.632711e+04

#define FP_DP_3STAGE_MUL_4ns_int_power                2.908127e+00
#define FP_DP_3STAGE_MUL_4ns_switch_power             5.000629e+00
#define FP_DP_3STAGE_MUL_4ns_dynamic_power            7.908756e+00
#define FP_DP_3STAGE_MUL_4ns_dynamic_energy           9.490508e+01
#define FP_DP_3STAGE_MUL_4ns_leakage_power            1.511790e-01
#define FP_DP_3STAGE_MUL_4ns_area                     1.624086e+04

#define FP_DP_3STAGE_MUL_5ns_int_power                2.253656e+00
#define FP_DP_3STAGE_MUL_5ns_switch_power             3.942233e+00
#define FP_DP_3STAGE_MUL_5ns_dynamic_power            6.195889e+00
#define FP_DP_3STAGE_MUL_5ns_dynamic_energy           9.293833e+01
#define FP_DP_3STAGE_MUL_5ns_leakage_power            1.402072e-01
#define FP_DP_3STAGE_MUL_5ns_area                     1.576422e+04

#define FP_DP_3STAGE_MUL_6ns_int_power                1.827799e+00
#define FP_DP_3STAGE_MUL_6ns_switch_power             3.205311e+00
#define FP_DP_3STAGE_MUL_6ns_dynamic_power            5.033110e+00
#define FP_DP_3STAGE_MUL_6ns_dynamic_energy           9.059598e+01
#define FP_DP_3STAGE_MUL_6ns_leakage_power            1.348457e-01
#define FP_DP_3STAGE_MUL_6ns_area                     1.536190e+04

#define FP_DP_3STAGE_MUL_10ns_int_power               9.056301e-01
#define FP_DP_3STAGE_MUL_10ns_switch_power            1.979930e+00
#define FP_DP_3STAGE_MUL_10ns_dynamic_power           2.885502e+00
#define FP_DP_3STAGE_MUL_10ns_dynamic_energy          8.656505e+01
#define FP_DP_3STAGE_MUL_10ns_leakage_power           1.232215e-01
#define FP_DP_3STAGE_MUL_10ns_area                    1.394492e+04

#define FP_3STAGE_TRIG_10ns_int_power                 1.746411e-01
#define FP_3STAGE_TRIG_10ns_switch_power              3.546680e-01
#define FP_3STAGE_TRIG_10ns_dynamic_power             5.293091e-01
#define FP_3STAGE_TRIG_10ns_dynamic_energy            1.587927e+01
#define FP_3STAGE_TRIG_10ns_leakage_power             1.037373e-01
#define FP_3STAGE_TRIG_10ns_area                      1.058682e+04

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
/* Power and area cost for a floating-point sin/cos functional unit.*/
void getTrigonometricFunctionPowerArea(float cycle_time,
                                      float* internal_power,
                                      float* switch_power,
                                      float* leakage_power,
                                      float* area);
/* Access cacti_interface() to calculate power/area. */
uca_org_t cactiWrapper(
  unsigned num_of_bytes, unsigned wordsize, unsigned num_ports);
#endif
