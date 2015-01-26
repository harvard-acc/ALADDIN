#ifndef __POWER_FUNC_H__
#define __POWER_FUNC_H__

#include <stdlib.h>
#include <iostream>

#define   ADD_LATENCY       1
#define   MEMOP_LATENCY     1
#define   MUL_LATENCY       1
#define   RW_PORTS          1
/* All the power numbers are in mW, energy numbers are in nJ */

/* Function units power model normalized to a comercial 40g */
/* MUL     : integer MUL,DIV model*/
/* ADD     : integer ADD,SUB model*/
/* BIT     : AND,OR,XOR model*/
/* SHIFTER : Shl,LShr,AShr model*/

#define	REG_1ns_int_power	7.936518e-03
#define	REG_1ns_switch_power	1.062977e-03
#define	REG_1ns_dynamic_power	8.999495e-03
#define	REG_1ns_dynamic_energy	8.999495e-03
#define	REG_1ns_leakage_power	7.395312e-05
#define	REG_1ns_area	5.981433e+00

#define	REG_2ns_int_power	3.967495e-03
#define	REG_2ns_switch_power	5.381930e-04
#define	REG_2ns_dynamic_power	4.505785e-03
#define	REG_2ns_dynamic_energy	9.011432e-03
#define	REG_2ns_leakage_power	7.395312e-05
#define	REG_2ns_area	5.981433e+00

#define	REG_3ns_int_power	2.644534e-03
#define	REG_3ns_switch_power	3.584390e-04
#define	REG_3ns_dynamic_power	3.002932e-03
#define	REG_3ns_dynamic_energy	9.008934e-03
#define	REG_3ns_leakage_power	7.395312e-05
#define	REG_3ns_area	5.981433e+00

#define	REG_4ns_int_power	1.982290e-03
#define	REG_4ns_switch_power	2.685621e-04
#define	REG_4ns_dynamic_power	2.250880e-03
#define	REG_4ns_dynamic_energy	9.003520e-03
#define	REG_4ns_leakage_power	7.395312e-05
#define	REG_4ns_area	5.981433e+00

#define	REG_5ns_int_power	1.586138e-03
#define	REG_5ns_switch_power	2.146359e-04
#define	REG_5ns_dynamic_power	1.800732e-03
#define	REG_5ns_dynamic_energy	9.003937e-03
#define	REG_5ns_leakage_power	7.395312e-05
#define	REG_5ns_area	5.981433e+00

#define	REG_6ns_int_power	1.322600e-03
#define	REG_6ns_switch_power	1.792126e-04
#define	REG_6ns_dynamic_power	1.501882e-03
#define	REG_6ns_dynamic_energy	9.011016e-03
#define	REG_6ns_leakage_power	7.395312e-05
#define	REG_6ns_area	5.981433e+00

#define	ADD_1ns_int_power	5.523790e-02
#define	ADD_1ns_switch_power	1.551852e-01
#define	ADD_1ns_dynamic_power	2.104162e-01
#define	ADD_1ns_dynamic_energy	2.104162e-01
#define	ADD_1ns_leakage_power	2.249908e-03
#define	ADD_1ns_area	2.779792e+02

#define	ADD_2ns_int_power	2.436880e-02
#define	ADD_2ns_switch_power	1.850702e-02
#define	ADD_2ns_dynamic_power	4.287582e-02
#define	ADD_2ns_dynamic_energy	8.575164e-02
#define	ADD_2ns_leakage_power	2.380803e-03
#define	ADD_2ns_area	1.794430e+02

#define	ADD_3ns_int_power	1.626669e-02
#define	ADD_3ns_switch_power	1.235638e-02
#define	ADD_3ns_dynamic_power	2.862321e-02
#define	ADD_3ns_dynamic_energy	8.586963e-02
#define	ADD_3ns_leakage_power	2.380803e-03
#define	ADD_3ns_area	1.794430e+02

#define	ADD_4ns_int_power	1.217552e-02
#define	ADD_4ns_switch_power	9.249207e-03
#define	ADD_4ns_dynamic_power	2.142472e-02
#define	ADD_4ns_dynamic_energy	8.569890e-02
#define	ADD_4ns_leakage_power	2.380803e-03
#define	ADD_4ns_area	1.794430e+02

#define	ADD_5ns_int_power	9.743773e-03
#define	ADD_5ns_switch_power	7.400587e-03
#define	ADD_5ns_dynamic_power	1.714394e-02
#define	ADD_5ns_dynamic_energy	8.572111e-02
#define	ADD_5ns_leakage_power	2.380803e-03
#define	ADD_5ns_area	1.794430e+02

#define	ADD_6ns_int_power	8.115300e-03
#define	ADD_6ns_switch_power	6.162853e-03
#define	ADD_6ns_dynamic_power	1.427760e-02
#define	ADD_6ns_dynamic_energy	8.566836e-02
#define	ADD_6ns_leakage_power	2.380803e-03
#define	ADD_6ns_area	1.794430e+02

#define	MUL_1ns_int_power	4.979808e+00
#define	MUL_1ns_switch_power	7.703601e+00
#define	MUL_1ns_dynamic_power	1.268341e+01
#define	MUL_1ns_dynamic_energy	1.268341e+01
#define	MUL_1ns_leakage_power	7.786052e-02
#define	MUL_1ns_area	6.351338e+03

#define	MUL_2ns_int_power	1.748818e+00
#define	MUL_2ns_switch_power	3.028611e+00
#define	MUL_2ns_dynamic_power	4.777429e+00
#define	MUL_2ns_dynamic_energy	9.554858e+00
#define	MUL_2ns_leakage_power	4.831147e-02
#define	MUL_2ns_area	5.137736e+03

#define	MUL_3ns_int_power	1.125718e+00
#define	MUL_3ns_switch_power	1.808644e+00
#define	MUL_3ns_dynamic_power	2.934361e+00
#define	MUL_3ns_dynamic_energy	8.803084e+00
#define	MUL_3ns_leakage_power	4.668189e-02
#define	MUL_3ns_area	4.974349e+03

#define	MUL_4ns_int_power	8.601815e-01
#define	MUL_4ns_switch_power	1.305749e+00
#define	MUL_4ns_dynamic_power	2.165931e+00
#define	MUL_4ns_dynamic_energy	8.663723e+00
#define	MUL_4ns_leakage_power	4.853217e-02
#define	MUL_4ns_area	4.619555e+03

#define	MUL_5ns_int_power	6.866739e-01
#define	MUL_5ns_switch_power	1.038409e+00
#define	MUL_5ns_dynamic_power	1.725082e+00
#define	MUL_5ns_dynamic_energy	8.625412e+00
#define	MUL_5ns_leakage_power	4.817683e-02
#define	MUL_5ns_area	4.595000e+03

#define	MUL_6ns_int_power	5.725752e-01
#define	MUL_6ns_switch_power	8.662890e-01
#define	MUL_6ns_dynamic_power	1.438864e+00
#define	MUL_6ns_dynamic_energy	8.633185e+00
#define	MUL_6ns_leakage_power	4.817683e-02
#define	MUL_6ns_area	4.595000e+03

#define	BIT_1ns_int_power	1.010606e-02
#define	BIT_1ns_switch_power	7.950398e-03
#define	BIT_1ns_dynamic_power	1.805590e-02
#define	BIT_1ns_dynamic_energy	1.805590e-02
#define	BIT_1ns_leakage_power	6.111633e-04
#define	BIT_1ns_area	5.036996e+01

#define	BIT_2ns_int_power	5.047962e-03
#define	BIT_2ns_switch_power	3.971243e-03
#define	BIT_2ns_dynamic_power	9.019205e-03
#define	BIT_2ns_dynamic_energy	1.803786e-02
#define	BIT_2ns_leakage_power	6.111633e-04
#define	BIT_2ns_area	5.036996e+01

#define	BIT_3ns_int_power	3.362023e-03
#define	BIT_3ns_switch_power	2.644812e-03
#define	BIT_3ns_dynamic_power	6.006835e-03
#define	BIT_3ns_dynamic_energy	1.801981e-02
#define	BIT_3ns_leakage_power	6.111633e-04
#define	BIT_3ns_area	5.036996e+01

#define	BIT_4ns_int_power	2.518915e-03
#define	BIT_4ns_switch_power	1.981596e-03
#define	BIT_4ns_dynamic_power	4.500511e-03
#define	BIT_4ns_dynamic_energy	1.800177e-02
#define	BIT_4ns_leakage_power	6.111633e-04
#define	BIT_4ns_area	5.036996e+01

#define	BIT_5ns_int_power	2.013105e-03
#define	BIT_5ns_switch_power	1.583778e-03
#define	BIT_5ns_dynamic_power	3.596883e-03
#define	BIT_5ns_dynamic_energy	1.798511e-02
#define	BIT_5ns_leakage_power	6.111633e-04
#define	BIT_5ns_area	5.036996e+01

#define	BIT_6ns_int_power	1.680942e-03
#define	BIT_6ns_switch_power	1.322420e-03
#define	BIT_6ns_dynamic_power	3.003348e-03
#define	BIT_6ns_dynamic_energy	1.801981e-02
#define	BIT_6ns_leakage_power	6.111633e-04
#define	BIT_6ns_area	5.036996e+01

#define	SHIFTER_1ns_int_power	3.414631e-01
#define	SHIFTER_1ns_switch_power	4.554229e-01
#define	SHIFTER_1ns_dynamic_power	7.968859e-01
#define	SHIFTER_1ns_dynamic_energy	7.968859e-01
#define	SHIFTER_1ns_leakage_power	4.048142e-03
#define	SHIFTER_1ns_area	5.197551e+02

#define	SHIFTER_2ns_int_power	1.729524e-01
#define	SHIFTER_2ns_switch_power	2.305570e-01
#define	SHIFTER_2ns_dynamic_power	4.035094e-01
#define	SHIFTER_2ns_dynamic_energy	8.070188e-01
#define	SHIFTER_2ns_leakage_power	4.054110e-03
#define	SHIFTER_2ns_area	5.197551e+02

#define	SHIFTER_3ns_int_power	1.154395e-01
#define	SHIFTER_3ns_switch_power	1.537972e-01
#define	SHIFTER_3ns_dynamic_power	2.692422e-01
#define	SHIFTER_3ns_dynamic_energy	8.077128e-01
#define	SHIFTER_3ns_leakage_power	4.063272e-03
#define	SHIFTER_3ns_area	5.197551e+02

#define	SHIFTER_4ns_int_power	8.696481e-02
#define	SHIFTER_4ns_switch_power	1.158184e-01
#define	SHIFTER_4ns_dynamic_power	2.027819e-01
#define	SHIFTER_4ns_dynamic_energy	8.111274e-01
#define	SHIFTER_4ns_leakage_power	4.065076e-03
#define	SHIFTER_4ns_area	5.197551e+02

#define	SHIFTER_5ns_int_power	6.979866e-02
#define	SHIFTER_5ns_switch_power	9.279467e-02
#define	SHIFTER_5ns_dynamic_power	1.625975e-01
#define	SHIFTER_5ns_dynamic_energy	8.129736e-01
#define	SHIFTER_5ns_leakage_power	4.069101e-03
#define	SHIFTER_5ns_area	5.197551e+02

#define	SHIFTER_6ns_int_power	5.823888e-02
#define	SHIFTER_6ns_switch_power	7.743299e-02
#define	SHIFTER_6ns_dynamic_power	1.356719e+00
#define	SHIFTER_6ns_dynamic_energy	8.140285e-01
#define	SHIFTER_6ns_leakage_power	4.069379e-03
#define	SHIFTER_6ns_area	5.197551e+02

void getRegisterPowerArea(float cycle_time, float *internal_power_per_bit,
                          float *switch_power_per_bit, float *leakage_power_per_bit,
                          float *area);
void getAdderPowerArea(float cycle_time, float *internal_power,
                         float *swich_power, float *leakage_power, float *area);
void getMultiplierPowerArea(float cycle_time, float *internal_power,
                         float *switch_power, float *leakage_power, float *area);
void getBitPowerArea(float cycle_time, float *internal_power,
                         float *swich_power, float *leakage_power, float *area);
void getShifterPowerArea(float cycle_time, float *internal_power,
                         float *swich_power, float *leakage_power, float *area);
#endif
