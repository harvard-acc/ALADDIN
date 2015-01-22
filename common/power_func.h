#ifndef __POWER_FUNC_H__
#define __POWER_FUNC_H__

#include <stdlib.h>
#include <iostream>

#define   ADD_LATENCY       1
#define   MEMOP_LATENCY     1
#define   MUL_LATENCY       1
#define   RW_PORTS          1

#define	REG_1ns_int_power	8.468589e-03
#define	REG_1ns_switch_power	6.897547e-04
#define	REG_1ns_dynamic_power	9.158399e-03
#define	REG_1ns_dynamic_energy	9.158399e-03
#define	REG_1ns_leakage_power	7.428256e-05
#define	REG_1ns_area	5.977338e+00

#define	REG_2ns_int_power	4.233601e-03
#define	REG_2ns_switch_power	3.449051e-04
#define	REG_2ns_dynamic_power	4.578437e-03
#define	REG_2ns_dynamic_energy	9.157012e-03
#define	REG_2ns_leakage_power	7.428256e-05
#define	REG_2ns_area	5.977338e+00

#define	REG_4ns_int_power	2.116731e-03
#define	REG_4ns_switch_power	1.724456e-04
#define	REG_4ns_dynamic_power	2.289288e-03
#define	REG_4ns_dynamic_energy	9.157012e-03
#define	REG_4ns_leakage_power	7.428256e-05
#define	REG_4ns_area	5.977338e+00

#define	REG_6ns_int_power	1.411385e-03
#define	REG_6ns_switch_power	1.149665e-04
#define	REG_6ns_dynamic_power	1.526377e-03
#define	REG_6ns_dynamic_energy	9.157983e-03
#define	REG_6ns_leakage_power	7.428256e-05
#define	REG_6ns_area	5.977338e+00

#define	ADD_1ns_int_power	5.495317e-02
#define	ADD_1ns_switch_power	1.550790e-01
#define	ADD_1ns_dynamic_power	2.100363e-01
#define	ADD_1ns_dynamic_energy	2.100363e-01
#define	ADD_1ns_leakage_power	2.252807e-03
#define	ADD_1ns_area	2.777889e+02

#define	ADD_2ns_int_power	2.276942e-02
#define	ADD_2ns_switch_power	1.819196e-02
#define	ADD_2ns_dynamic_power	4.096138e-02
#define	ADD_2ns_dynamic_energy	8.192277e-02
#define	ADD_2ns_leakage_power	2.373902e-03
#define	ADD_2ns_area	1.790055e+02

#define	ADD_4ns_int_power	1.207244e-02
#define	ADD_4ns_switch_power	9.248146e-03
#define	ADD_4ns_dynamic_power	2.132128e-02
#define	ADD_4ns_dynamic_energy	8.528235e-02
#define	ADD_4ns_leakage_power	2.379173e-03
#define	ADD_4ns_area	1.793201e+02

#define	ADD_6ns_int_power	8.050098e-03
#define	ADD_6ns_switch_power	6.167095e-03
#define	ADD_6ns_dynamic_power	1.421650e-02
#define	ADD_6ns_dynamic_energy	8.530316e-02
#define	ADD_6ns_leakage_power	2.379173e-03
#define	ADD_6ns_area	1.793201e+02

#define	MUL_1ns_int_power	5.213734e+00
#define	MUL_1ns_switch_power	8.126250e+00
#define	MUL_1ns_dynamic_power	1.333998e+01
#define	MUL_1ns_dynamic_energy	1.333998e+01
#define	MUL_1ns_leakage_power	8.294646e-02
#define	MUL_1ns_area	6.537320e+03

#define	MUL_2ns_int_power	1.792286e+00
#define	MUL_2ns_switch_power	3.384689e+00
#define	MUL_2ns_dynamic_power	5.176975e+00
#define	MUL_2ns_dynamic_energy	1.035395e+01
#define	MUL_2ns_leakage_power	4.961834e-02
#define	MUL_2ns_area	5.181094e+03

#define	MUL_4ns_int_power	8.855316e-01
#define	MUL_4ns_switch_power	1.432331e+00
#define	MUL_4ns_dynamic_power	2.317862e+00
#define	MUL_4ns_dynamic_energy	9.271449e+00
#define	MUL_4ns_leakage_power	5.034241e-02
#define	MUL_4ns_area	4.722097e+03

#define	MUL_6ns_int_power	5.762059e-01
#define	MUL_6ns_switch_power	8.888606e-01
#define	MUL_6ns_dynamic_power	1.465066e+00
#define	MUL_6ns_dynamic_energy	8.790399e+00
#define	MUL_6ns_leakage_power	4.836856e-02
#define	MUL_6ns_area	4.595000e+03

#define	BIT_1ns_int_power	1.010815e-02
#define	BIT_1ns_switch_power	7.952168e-03
#define	BIT_1ns_dynamic_power	1.806018e-02
#define	BIT_1ns_dynamic_energy	1.806018e-02
#define	BIT_1ns_leakage_power	6.107449e-04
#define	BIT_1ns_area	5.033548e+01

#define	BIT_2ns_int_power	5.054493e-03
#define	BIT_2ns_switch_power	3.976431e-03
#define	BIT_2ns_dynamic_power	9.030924e-03
#define	BIT_2ns_dynamic_energy	1.806157e-02
#define	BIT_2ns_leakage_power	6.107449e-04
#define	BIT_2ns_area	5.033548e+01

#define	BIT_4ns_int_power	2.527177e-03
#define	BIT_4ns_switch_power	1.988146e-03
#define	BIT_4ns_dynamic_power	4.515323e-03
#define	BIT_4ns_dynamic_energy	1.806157e-02
#define	BIT_4ns_leakage_power	6.107449e-04
#define	BIT_4ns_area	5.033548e+01

#define	BIT_6ns_int_power	1.684785e-03
#define	BIT_6ns_switch_power	1.325454e-03
#define	BIT_6ns_dynamic_power	3.010308e-03
#define	BIT_6ns_dynamic_energy	1.806157e-02
#define	BIT_6ns_leakage_power	6.107449e-04
#define	BIT_6ns_area	5.033548e+01

#define	SHIFTER_1ns_int_power	3.680005e-01
#define	SHIFTER_1ns_switch_power	5.498507e-01
#define	SHIFTER_1ns_dynamic_power	9.178513e-01
#define	SHIFTER_1ns_dynamic_energy	9.178513e-01
#define	SHIFTER_1ns_leakage_power	4.130816e-03
#define	SHIFTER_1ns_area	5.193992e+02

#define	SHIFTER_2ns_int_power	1.799083e-01
#define	SHIFTER_2ns_switch_power	2.699318e-01
#define	SHIFTER_2ns_dynamic_power	4.498401e-01
#define	SHIFTER_2ns_dynamic_energy	8.996801e-01
#define	SHIFTER_2ns_leakage_power	4.118748e-03
#define	SHIFTER_2ns_area	5.193992e+02

#define	SHIFTER_4ns_int_power	9.296417e-02
#define	SHIFTER_4ns_switch_power	1.384891e-01
#define	SHIFTER_4ns_dynamic_power	2.314533e-01
#define	SHIFTER_4ns_dynamic_energy	9.258133e-01
#define	SHIFTER_4ns_leakage_power	4.124297e-03
#define	SHIFTER_4ns_area	5.193992e+02

#define	SHIFTER_6ns_int_power	6.213979e-02
#define	SHIFTER_6ns_switch_power	9.252584e-02
#define	SHIFTER_6ns_dynamic_power	1.546629e-01
#define	SHIFTER_6ns_dynamic_energy	9.279910e-01
#define	SHIFTER_6ns_leakage_power	4.125823e-03
#define	SHIFTER_6ns_area	5.193992e+02

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
