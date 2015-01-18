#ifndef __POWER_FUNC_H__
#define __POWER_FUNC_H__

#include <stdlib.h>
#include <iostream>

#define   ADD_LATENCY       1
#define   MEMOP_LATENCY     1
#define   MUL_LATENCY       1
#define   RW_PORTS          1
/* All the power numbers are in mW. */

#define	ADD_1ns_int_power	1.460572e-02
#define	ADD_1ns_switch_power	3.829290e-02
#define	ADD_1ns_leak_power	3.911312e-03
#define	ADD_1ns_area	4.901333e+02
#define	REG_1ns_int_power	1.401167e-03
#define	REG_1ns_switch_power	1.849026e-04
#define	REG_1ns_leak_power	1.308388e-04
#define	REG_1ns_area	1.058242e+01
#define	MUL_1ns_int_power	1.194489e+00
#define	MUL_1ns_switch_power	1.613690e+00
#define	MUL_1ns_leak_power	5.877406e-02
#define	MUL_1ns_area	5.177590e+03

#define	ADD_2ns_int_power	1.239184e-02
#define	ADD_2ns_switch_power	9.351345e-03
#define	ADD_2ns_leak_power	4.212390e-03
#define	ADD_2ns_area	3.174727e+02
#define	REG_2ns_int_power	1.401241e-03
#define	REG_2ns_switch_power	1.889080e-04
#define	REG_2ns_leak_power	1.308388e-04
#define	REG_2ns_area	1.058242e+01
#define	MUL_2ns_int_power	8.501892e-01
#define	MUL_2ns_switch_power	1.030198e+00
#define	MUL_2ns_leak_power	5.218030e-02
#define	MUL_2ns_area	4.663507e+03

#define	ADD_3ns_int_power	1.170963e-02
#define	ADD_3ns_switch_power	8.861663e-03
#define	ADD_3ns_leak_power	4.212390e-03
#define	ADD_3ns_area	3.174727e+02
#define	REG_3ns_int_power	1.401241e-03
#define	REG_3ns_switch_power	1.889080e-04
#define	REG_3ns_leak_power	1.308388e-04
#define	REG_3ns_area	1.058242e+01
#define	MUL_3ns_int_power	7.050529e-01
#define	MUL_3ns_switch_power	8.745014e-01
#define	MUL_3ns_leak_power	5.093277e-02
#define	MUL_3ns_area	4.595000e+03

#define	ADD_4ns_int_power	1.108144e-02
#define	ADD_4ns_switch_power	8.402433e-03
#define	ADD_4ns_leak_power	4.212145e-03
#define	ADD_4ns_area	3.174727e+02
#define	REG_4ns_int_power	1.401241e-03
#define	REG_4ns_switch_power	1.889080e-04
#define	REG_4ns_leak_power	1.308388e-04
#define	REG_4ns_area	1.058242e+01
#define	MUL_4ns_int_power	6.112423e-01
#define	MUL_4ns_switch_power	7.826554e-01
#define	MUL_4ns_leak_power	5.092786e-02
#define	MUL_4ns_area	4.595000e+03

#define	ADD_5ns_int_power	1.053208e-02
#define	ADD_5ns_switch_power	7.995020e-03
#define	ADD_5ns_leak_power	4.212390e-03
#define	ADD_5ns_area	3.174727e+02
#define	REG_5ns_int_power	1.401241e-03
#define	REG_5ns_switch_power	1.889080e-04
#define	REG_5ns_leak_power	1.308388e-04
#define	REG_5ns_area	1.058242e+01
#define	MUL_5ns_int_power	5.405160e-01
#define	MUL_5ns_switch_power	7.116835e-01
#define	MUL_5ns_leak_power	5.093031e-02
#define	MUL_5ns_area	4.595000e+03

#define	ADD_6ns_int_power	1.006327e-02
#define	ADD_6ns_switch_power	7.636477e-03
#define	ADD_6ns_leak_power	4.212390e-03
#define	ADD_6ns_area	3.174727e+02
#define	REG_6ns_int_power	1.401241e-03
#define	REG_6ns_switch_power	1.889080e-04
#define	REG_6ns_leak_power	1.308388e-04
#define	REG_6ns_area	1.058242e+01
#define	MUL_6ns_int_power	4.859978e-01
#define	MUL_6ns_switch_power	6.554463e-01
#define	MUL_6ns_leak_power	5.093031e-02
#define	MUL_6ns_area	4.595000e+03

void getAdderPowerArea(float cycle_time, float *internal_power,
                   float *swich_power, float *leakage_power, float *area);
void getMultiplierPowerArea(float cycle_time, float *internal_power,
                        float *switch_power, float *leakage_power, float *area);
void getRegisterPowerArea(float cycle_time, float *internal_power_per_bit,
                      float *switch_power_per_bit, float *leakage_power_per_bit,
                      float *area);
#endif
