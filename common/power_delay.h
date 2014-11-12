#ifndef __POWER_DELAY_H__
#define __POWER_DELAY_H__

#define   CYCLE_TIME        1
#define   ADD_LATENCY       1
#define   MEMOP_LATENCY     1
#define   MUL_LATENCY       1

#define   RW_PORTS          1
/* Function units power model with TSMC 40g @ 1GHz. */
#define   ADD_int_power     0.0059475
#define   ADD_switch_power  0.015593
#define   ADD_leak_power    0.0015927
#define   ADD_area          199.583996
#define   MUL_int_power     0.4864
#define   MUL_switch_power  0.6571
#define   MUL_leak_power    0.0239328
#define   MUL_area          2108.332748
#define   REG_int_power     0.0005705625
#define   REG_sw_power      0.0000752927
#define   REG_leak_power    0.0000053278
#define   REG_area          4.309200

#endif
