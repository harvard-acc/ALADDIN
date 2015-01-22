#ifndef __POWER_FUNC_H__
#define __POWER_FUNC_H__

#include <stdlib.h>
#include <iostream>

#define   ADD_LATENCY       1
#define   MEMOP_LATENCY     1
#define   MUL_LATENCY       1
#define   RW_PORTS          1
/* All the power numbers are in mW, energy numbers are in nJ */

/* Function units power model with TSMC 40g */
#define	REG_1ns_int_power         6.1052e-03
#define	REG_1ns_switch_power      4.9726e-04
#define	REG_1ns_dynamic_power     6.6025e-03
#define	REG_1ns_dynamic_energy    6.6025e-03
#define	REG_1ns_leakage_power     5.3552e-05
#define	REG_1ns_area              4.309200

#define	REG_2ns_int_power         3.0521e-03
#define	REG_2ns_switch_power      2.4865e-04
#define	REG_2ns_dynamic_power     3.3007e-03
#define	REG_2ns_dynamic_energy    6.6015e-03
#define	REG_2ns_leakage_power     5.3552e-05
#define	REG_2ns_area              4.309200

#define	REG_4ns_int_power         1.5260e-03
#define	REG_4ns_switch_power      1.2432e-04
#define	REG_4ns_dynamic_power     1.6504e-03
#define	REG_4ns_dynamic_energy    6.6015e-03
#define	REG_4ns_leakage_power     5.3552e-05
#define	REG_4ns_area              4.309200

#define	REG_6ns_int_power         1.0175e-03
#define	REG_6ns_switch_power      8.2882e-05
#define	REG_6ns_dynamic_power     1.1004e-03
#define	REG_6ns_dynamic_energy    6.6022e-03
#define	REG_6ns_leakage_power     5.3552e-05
#define	REG_6ns_area              4.309200

#define	ADD_1ns_int_power         3.9617e-02
#define	ADD_1ns_switch_power      1.1180e-01
#define	ADD_1ns_dynamic_power     1.5142e-01
#define	ADD_1ns_dynamic_energy    1.5142e-01
#define	ADD_1ns_leakage_power     1.6241e-03
#define	ADD_1ns_area              200.264396

#define	ADD_2ns_int_power         1.6415e-02
#define	ADD_2ns_switch_power      1.3115e-02
#define	ADD_2ns_dynamic_power     2.9530e-02
#define	ADD_2ns_dynamic_energy    5.9060e-02
#define	ADD_2ns_leakage_power     1.7114e-03
#define	ADD_2ns_area              129.049195

#define	ADD_4ns_int_power         8.7033e-03
#define	ADD_4ns_switch_power      6.6672e-03
#define	ADD_4ns_dynamic_power     1.5371e-02
#define	ADD_4ns_dynamic_energy    6.1482e-02
#define	ADD_4ns_leakage_power     1.7152e-03
#define	ADD_4ns_area              129.275995

#define	ADD_6ns_int_power         5.8035e-03
#define	ADD_6ns_switch_power      4.4460e-03
#define	ADD_6ns_dynamic_power     1.0249e-02
#define	ADD_6ns_dynamic_energy    6.1497e-02
#define	ADD_6ns_leakage_power     1.7152e-03
#define	ADD_6ns_area              129.275995

#define	MUL_1ns_int_power         3.7587e+00
#define	MUL_1ns_switch_power      5.8584e+00
#define	MUL_1ns_dynamic_power     9.6171e+00
#define	MUL_1ns_dynamic_energy    9.6171e+00
#define	MUL_1ns_leakage_power     5.9798e-02
#define	MUL_1ns_area              4712.903878

#define	MUL_2ns_int_power         1.2921e+00
#define	MUL_2ns_switch_power      2.4401e+00
#define	MUL_2ns_dynamic_power     3.7322e+00
#define	MUL_2ns_dynamic_energy    7.4644e+00
#define	MUL_2ns_leakage_power     3.5771e-02
#define	MUL_2ns_area              3735.169181

#define	MUL_4ns_int_power         6.3840e-01
#define	MUL_4ns_switch_power      1.0326e+00
#define	MUL_4ns_dynamic_power     1.6710e+00
#define	MUL_4ns_dynamic_energy    6.6840e+00
#define	MUL_4ns_leakage_power     3.6293e-02
#define	MUL_4ns_area              3404.267980

#define	MUL_6ns_int_power         4.1540e-01
#define	MUL_6ns_switch_power      6.4080e-01
#define	MUL_6ns_dynamic_power     1.0562e+00
#define	MUL_6ns_dynamic_energy    6.3372e+00
#define	MUL_6ns_leakage_power     3.4870e-02
#define	MUL_6ns_area              3312.640779

#define	BIT_1ns_int_power         7.2872e-03
#define	BIT_1ns_switch_power      5.7329e-03
#define	BIT_1ns_dynamic_power     1.3020e-02
#define	BIT_1ns_dynamic_energy    1.3020e-02
#define	BIT_1ns_leakage_power     4.4030e-04
#define	BIT_1ns_area              36.287998

#define	BIT_2ns_int_power         3.6439e-03
#define	BIT_2ns_switch_power      2.8667e-03
#define	BIT_2ns_dynamic_power     6.5106e-03
#define	BIT_2ns_dynamic_energy    1.3021e-02
#define	BIT_2ns_leakage_power     4.4030e-04
#define	BIT_2ns_area              36.287998

#define	BIT_4ns_int_power         1.8219e-03
#define	BIT_4ns_switch_power      1.4333e-03
#define	BIT_4ns_dynamic_power     3.2552e-03
#define	BIT_4ns_dynamic_energy    1.3021e-02
#define	BIT_4ns_leakage_power     4.4030e-04
#define	BIT_4ns_area              36.287998

#define	BIT_6ns_int_power         1.2146e-03
#define	BIT_6ns_switch_power      9.5555e-04
#define	BIT_6ns_dynamic_power     2.1702e-03
#define	BIT_6ns_dynamic_energy    1.3021e-02
#define	BIT_6ns_leakage_power     4.4030e-04
#define	BIT_6ns_area              36.287998

#define	SHIFTER_1ns_int_power         2.6530e-01
#define	SHIFTER_1ns_switch_power      3.9640e-01
#define	SHIFTER_1ns_dynamic_power     6.6170e-01
#define	SHIFTER_1ns_dynamic_energy    6.6170e-01
#define	SHIFTER_1ns_leakage_power     2.9780e-03
#define	SHIFTER_1ns_area              374.446796

#define	SHIFTER_2ns_int_power         1.2970e-01
#define	SHIFTER_2ns_switch_power      1.9460e-01
#define	SHIFTER_2ns_dynamic_power     3.2430e-01
#define	SHIFTER_2ns_dynamic_energy    6.4860e-01
#define	SHIFTER_2ns_leakage_power     2.9693e-03
#define	SHIFTER_2ns_area              374.446796

#define	SHIFTER_4ns_int_power         6.7020e-02
#define	SHIFTER_4ns_switch_power      9.9840e-02
#define	SHIFTER_4ns_dynamic_power     1.6686e-01
#define	SHIFTER_4ns_dynamic_energy    6.6744e-01
#define	SHIFTER_4ns_leakage_power     2.9733e-03
#define	SHIFTER_4ns_area              374.446796

#define	SHIFTER_6ns_int_power         4.4798e-02
#define	SHIFTER_6ns_switch_power      6.6704e-02
#define	SHIFTER_6ns_dynamic_power     1.1150e-01
#define	SHIFTER_6ns_dynamic_energy    6.6901e-01
#define	SHIFTER_6ns_leakage_power     2.9744e-03
#define	SHIFTER_6ns_area              374.446796

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
