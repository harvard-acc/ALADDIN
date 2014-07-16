#!/usr/bin/python
import numpy as np
import sys
import os
import glob
import operator
import gzip
import math
import math

class data:
    id = 0
    bits = 72
    words = 2048
    rd_power = 10.6
    wr_power = 12.3
    lk_power = 0.41
    area = 1000000

def main(kernel, config_file, header_file=''):
  BENCH_HOME = os.getenv('BENCH_HOME')
  BaseFile = BENCH_HOME + '/' + kernel
  ALADDIN_HOME = os.getenv('ALADDIN_HOME')

  print 'Running parse_power_per_run_ss.main()'

  #parse header for data type
  try:
    header = os.path.join(BaseFile, header_file)
    head = open(header, 'r')
    int_bits = 0
    frac_bits = 0
    for h in head:
      if 'NUM_OF_INT_BITS' in h:
        int_bits  = int(h.split()[2])
      if 'NUM_OF_FRAC_BITS' in h:
        frac_bits = int(h.split()[2])
    head.close()
  except:
    int_bits = 32
    frac_bits = 0

  # For now, everything is 32b
  total_bits = int_bits + frac_bits

  conf_data = config_file.split('_')
  config_fil = "%s_%s.config" % (conf_data[1], conf_data[2])
  f = open(config_fil, 'r')

  arrays = {}
  array_regs = []

  LL = []
  for line in f:
    LL.append(line)
  f.close()

  for line in LL:
    d = line.split(',')
    if 'complete' not in line and 'unrolling' not in line:
       #arrays[int(d[1])] = int(math.pow(2, math.ceil(math.log(int(d[3]), 2))))/int(d[4])
       arrays[int(d[1])] = 0
       #int(math.pow(2, math.ceil(math.log(int(d[3]), 2))))/int(d[4])

  for line in open(config_file, 'r'):
    if 'complete' in line:
       array_regs.append(d[2])
  scripts_home = "/group/brooks/shao/Projects/AccAnaFramework/ALADDIN/test/scripts"
  mem_file_name = os.path.join(scripts_home, "cacti_data.txt")
  #mem_file_name = os.path.join(ALADDIN_HOME, "cacti_data.txt")
  mem_f = open(mem_file_name, 'r')

  mem_choices = []
  for line in mem_f:
    mem_conf = line.strip(',').split()
    mem_conf[1].strip(',')
    if int(mem_conf[1]) == int(total_bits):
        mem_choices.append(line)
  mem_f.close()

  mems = {}
  for array in arrays:
    id = array
    size = arrays[array]
    DD = data()
    mems[id] = DD
    for mm in mem_choices:
        MM = mm.strip(',').split()
        this_data = data()
        this_data.id = id
        this_data.bits = MM[1]
        this_data.words = MM[3]
        this_data.area = MM[5]
        this_data.rd_power = MM[7]
        this_data.wr_power = MM[9]
        this_data.lk_power = MM[11]
        if int(this_data.words) == int(size):
            if float(this_data.area) < float(mems[id].area):
                mems[id] = this_data

  mem_regs = 0
  for m in array_regs:
      mem_regs += int(m)

  #Power numbers from 45nm openPDK
  #registers...
  REGint  = 0.0018
  REGsw   = 0.00013
  REGleak = 0.000055

  r_area = 7.98

  #adders...
  ADD_int    = 0.0151
  ADD_switch = 0.00615
  ADD_leak   = 0.00000129

  a_area     = 280

  #mults...
  MULT_int    = 0.0427
  MULT_switch = 0.0333
  MULT_leak   = 0.000025

  m_area      = 4595

  bitwidth = total_bits
  reg_stats_file = kernel + ".cil_reg_stats"
  reg_file = open(reg_stats_file)

  reg_write_32 = []
  reg_write_index = []
  reg_read = []
  reg_count = []

  for line in reg_file.readlines():
    parts = line.split(',')
    reg_write_32.append(int(parts[2]))
    reg_read.append(int(parts[1]))
  reg_file.close()

  max_written = max(reg_write_32) * 32
  max_read = max(reg_read) * 32
  max_reg = max_written + max_read + mem_regs
  
  bm_stats_file = kernel + ".cil_stats"
  activity_file = open(bm_stats_file, 'r')
  cycle = int(activity_file.readline().split(',')[1])

  cycles = np.zeros(cycle)
  mul_activity = np.zeros(cycle)
  add_activity = np.zeros(cycle)
  index_add_activity = np.zeros(cycle)
  shift_activity = np.zeros(cycle)

  ld_activity = {}
  st_activity = {}

  #from the second row
  for line in activity_file.readlines():
      if 'mul' in line:
          addr_line = line.strip()
          break
  activity_file.close()
  base_addresses = addr_line.split('add,')[1].split(',')
  base_addresses = base_addresses[:-1]

  #In order corresponding to stats file
  #simply walk through memory columns and read
  #dynamic power values
  base_power = []
  #One value for each mem
  #apply at the end
  mem_lk_stats   = []
  mem_area_stats = []
  total_mem_lk   = 0
  total_mem_area = 0

  for addr in base_addresses:
        addr_indx = int(addr.split('-')[0])
        base_power.append(mems[addr_indx].rd_power)
        base_power.append(mems[addr_indx].wr_power)
        mem_lk_stats.append(mems[addr_indx].lk_power)
        mem_area_stats.append(mems[addr_indx].area)
        total_mem_lk   += float(mems[addr_indx].lk_power)
        total_mem_area += float(mems[addr_indx].area)

  num_of_addrs = len(base_addresses)
  for k in range(len(base_addresses)):
    ld_activity[k] = np.zeros(cycle)
    st_activity[k] = np.zeros(cycle)

  #activity
  i = 0
  m_sum = 0
  max_mul = 0
  max_add = 0
  max_index = 0
  max_ld = np.zeros(num_of_addrs)
  max_st = np.zeros(num_of_addrs)
  total_mem_power = 0

  bm_act_file = kernel + ".cil_stats"
  activity_file = open(bm_act_file, 'r')
  for line in activity_file.readlines():
    if('cycles' in line):
      continue
    if('mul' in line):
      continue
    parts = line.strip().split(',')
    cycles[i] = int(parts[0])
    mul_activity[i] = int(parts[1])
    m_sum += mul_activity[i]
    add_activity[i] = int(parts[2])

    ii = 3
    while ii < len(parts)-1:
      total_mem_power += float(parts[ii])*float(base_power[ii-3])
      ii += 1
      total_mem_power += float(parts[ii])*float(base_power[ii-3])
      ii += 1
    i +=1
  activity_file.close()

  N = len(cycles)
  mult_per = float(m_sum)/float(N)
  max_mul = max(mul_activity)
  max_add = max(add_activity)
  max_index = max(index_add_activity)

  avg_mul_power = 0
  avg_add_power = 0
  avg_index_power = 0
  avg_reg_power = 0
  mul_internal = 0
  add_internal = 0

  #switching
  for i in range(N):
    switch_mul_power = float(MULT_switch) * mul_activity[i]
    switch_add_power = float(ADD_switch) * add_activity[i]
    switch_index_power = float(ADD_switch) * bitwidth / 32 * index_add_activity[i]
    reg_counts = reg_write_32[i] * 32 + reg_read[i] * 32
    switch_reg_power = float(REGsw) * reg_counts
    avg_mul_power += switch_mul_power
    avg_add_power += switch_add_power
    avg_index_power += switch_index_power
    avg_reg_power += switch_reg_power

  avg_reg_power    = (avg_reg_power / N)
  avg_add_power    = (avg_add_power / N)
  avg_mul_power    = (avg_mul_power / N)
  avg_index_power  = (avg_index_power / N)

  switch_compute   = avg_mul_power + avg_add_power + avg_index_power

  #internal
  mul_internal = MULT_int*math.ceil(float(m_sum)/float(N))
  add_internal = ADD_int*math.ceil(float(add_internal)/float(N))
  reg_internal = max_reg * float(REGint)
  index_internal = max_index*(float(ADD_int))*bitwidth

  internal_compute = mul_internal + add_internal + index_internal

  #leakage
  reg_leakage = max_reg * float(REGleak)
  mul_leakage = max_mul * float(MULT_leak)
  add_leakage = float(max_add * (ADD_leak))
  index_leakage = max_index * (float(ADD_leak) )*bitwidth 

  leakage_compute = mul_leakage + add_leakage + index_leakage

  avg_total_power = switch_compute + avg_reg_power + leakage_compute + \
                        reg_leakage + internal_compute + reg_internal

  #Report Stats
  print "swtich_compute: %s" % str(switch_compute)
  print "avg_reg_power: %s" % str(avg_reg_power)
  print "leakage_compute: %s" % str(leakage_compute)
  print "reg_leakage: %s" % str(reg_leakage)
  print "internal_conpute: %s" % str(internal_compute)
  print "reg_internal: %s" % str(reg_internal)
  AREA = (m_area*max_mul) + (a_area * max_add) + (max_reg * r_area)
  f = open('power.txt', 'w+')
  res = ["Max regs = %d\n" % max_reg,
         "Max Addr = %d\n" % max_add,
         "Max Mul = %d\n" % max_mul,
         "Area = %s\n" % str(AREA),
         "Power = %s\n" % str(avg_total_power),
         "Reg Power = %s\n" % str(avg_reg_power),
         "Add Power = %s\n" % str(avg_add_power),
         "Mul Power = %s\n" % str(avg_mul_power),
         "Cycles = %s\n" % str(N)]
  for r in res:
      f.write(r)
  f.close()
 
  print "\n .: Power Stats :. \n"
  print "Total Data Path Power == " + str(avg_total_power)
  print "Avg Mem Power == %f" % (float(total_mem_power)/float(len(cycles)))
  print "Mem lk = %f\n" % total_mem_lk

  print "\n .: Area Stats :. \n"
  print "Mem Area == " + str(total_mem_area)
  print "Total Data Path Area == " + str(AREA)
  print "\n"
  
  avg_total_power += (total_mem_lk + (float(total_mem_power)/float(len(cycles))))
  AREA += total_mem_area

  return avg_total_power, AREA, N

if __name__ == '__main__':
  if(len(sys.argv) != 3 ):
    sys.exit(0)
  kernel = sys.argv[1]
  config_file = sys.argv[2]
  main(kernel, config_file)


