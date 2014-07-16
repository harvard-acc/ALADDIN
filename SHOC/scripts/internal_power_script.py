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

def c_f(X):
    return 13.8 + 4*X +3*X*X

def b_f(X):
    return 4 - 0.4*X + 0.003*X*X

def a_f(X):
    return -1.7 + 0.026*X - 0.0003*X*X

def leak_base(X):
  return -0.43333333 + 0.34245968*X + 0.02599966*X*X

def int_base(X):
  return 0.00616667 - 0.00036915*X + 0.0003264*X*X

def sw_base(X):
  return -0.00066667 + 0.00084758*X + 0.00041415*X*X

def add_area(X):
    return -2.33333333 +  4.16895161*X + 0.00109206989*X*X

def add_int_power(X):
    return 1.00378138e-17 + 5.00000000e-04*X + 6.56113905e-21*X*X

def add_sw_power(X):
    return 1.00378138e-17 + 6.25000000e-04*X + 4.92085429e-21*X*X

def add_lk_power(X):
    return 1.04560560e-18 + 9.37500000e-05*X + 1.43524917e-21*X*X

def m8_area_off_first(X):
    return 425*X + 4570

def m8_rd_off_first(X):
    return .743*math.exp(.0225*X)

def m8_wr_off_first(X):
    return .683*math.exp(.0226*X)

def m8_lk_off_first(X):
    return .0568*math.exp(.0202*X)

def area_m8_first(X):
    return 10.71*X + m8_area_off_first(X)

def rd_m8_first(X):
    return .00021*X + m8_rd_off_first(X)

def wr_m8_first(X):
    return .00027*X + m8_wr_off_first(X)

def lk_m8_first(X):
    return .0001*X + m8_lk_off_first(X)

def area_m8_second(X):
    return 18581*math.exp(.0219*X)

def rd_m8_second(X):
    return .956*math.exp(.0203*X)

def wr_m8_second(X):
    return .969*math.exp(.0200*X)

def lk_m8_second(X):
    return .155*math.exp(.0208*X)

def area_m16(X):
    return 3290*X + 4518

def rd_m16(X):
    return .0937*X + .26

def wr_m16(X):
    return .0854*X + .271

def lk_m16(X):
    return .0257*X + .0662

def area_m32(X):
    return 4296*X + 184

def rd_m32(X):
    return .0949*X + .0333

def wr_m32(X):
    return .0832*X - .0382

def lk_m32(X):
    return .015*X + .0292

def main(kernel, config_file, header_file=''):
  BENCH_HOME = os.getenv('BENCH_HOME')
  BaseFile = BENCH_HOME + '/' + kernel
  
  ALADDIN_HOME = os.getenv('ALADDIN_HOME')

  print 'Running parse_power_per_run_ss.main()'

  #parse header for data type
  #header = os.path.join(BENCH_HOME, 'nnet_fwd.h')
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

  total_bits = int_bits + frac_bits

  #read config file
  bench = kernel
  #bench = 'nnet'

  conf_data = config_file.split('_')
  config_fil = "%s_%s.config" % (conf_data[1], conf_data[2])
  #"p32_u32.config"
  f = open(config_fil, 'r')
  arrays = {}
  array_regs = []
  cur_p = os.getcwd()

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

  #mem numbers, use total_bits, then words, then take min area
  #sram
  mem_file_name = ALADDIN_HOME + "/power-model/mem_dict.txt"
  #cacti
  #mem_file_name = os.path.join(ALADDIN_HOME, "cacti_real.txt")
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

  #REGS...
  REGint  = .000353
  REGsw   = .000103
  REGleak = .0000539
  r_area = 3.85
  #adder
  ADD_int    = add_int_power(total_bits)
  ADD_switch = add_sw_power(total_bits)
  ADD_leak   = add_lk_power(total_bits)
  a_area = add_area(total_bits)

  ADD_int    = add_int_power(total_bits)
  ADD_switch = add_sw_power(total_bits)
  ADD_leak   = add_lk_power(total_bits)
  a_area = add_area(total_bits)
  #MULTS...
  MULT_leak   = (leak_base(total_bits) + .162817607*frac_bits    - .0166875867*frac_bits*frac_bits)/(1000.0)
  MULT_int    = int_base(total_bits)  + .000996975806*frac_bits - .000172421035*frac_bits*frac_bits
  MULT_switch = sw_base(total_bits)   - .00558064516*frac_bits  - .000136*frac_bits*frac_bits
  m_area = (a_f(total_bits)*frac_bits*frac_bits) + (b_f(total_bits))*frac_bits + c_f(total_bits)

  bitwidth = total_bits
  reg_stats_file = kernel + ".cil_reg_stats"
  reg_file = open(reg_stats_file)
  #reg_file = open('nnet.cil_reg_stats')

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
  #activity_file = open('nnet.cil_stats', 'r')
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
  #activity_file = open('nnet.cil_stats', 'r')
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
      #ld_activity[ii-3][i] = float(parts[ii])*float(base_power[ii])
      total_mem_power += float(parts[ii])*float(base_power[ii-3])
      ii += 1
      #st_activity[ii-3][i] = float(parts[ii])*float(base_power[ii])
      total_mem_power += float(parts[ii])*float(base_power[ii-3])
      ii += 1
    i +=1
  activity_file.close()

  N = len(cycles)
  mult_per = float(m_sum)/float(N)
  max_mul = max(mul_activity)
  max_add = max(add_activity)
  max_index = max(index_add_activity)

  #leakage
  mul_leakage = max_mul * float(MULT_leak)
  add_leakage = float(max_add * (ADD_leak))
  index_leakage = max_index * (float(ADD_leak) )* bitwidth / 32
  leakage_compute = mul_leakage + add_leakage + index_leakage
  reg_leakage = max_reg * float(REGleak)

  #internal
  mul_internal = mult_per * float( MULT_int)
  add_internal = float(max_add * ( ADD_int))
  index_internal = max_index * (float(ADD_int) )* bitwidth / 32
  reg_internal = max_reg * float(REGint)

  avg_mul_power = 0
  avg_add_power = 0
  avg_index_power = 0
  avg_reg_power = 0
  avg_total_power = 0
  mul_internal = 0
  add_internal = 0

  #switching
  for i in range(N):
    mul_internal += mul_activity[i] * float( MULT_int)
    add_internal += add_activity[i] * float( ADD_int)
    switch_mul_power = float(MULT_switch) * mul_activity[i]
    switch_add_power = float(ADD_switch) * add_activity[i]
    switch_index_power = float(ADD_switch) * bitwidth / 32 * index_add_activity[i]
    reg_counts = reg_write_32[i] * 32 + reg_read[i] * 32
    switch_reg_power = float(REGsw) * reg_counts
    switch_compute = switch_mul_power + switch_add_power + switch_index_power
    avg_mul_power += switch_mul_power
    avg_add_power += switch_add_power
    avg_index_power += switch_index_power
    avg_reg_power += switch_reg_power

  mul_internal = MULT_int*math.ceil(float(m_sum)/float(N))
  add_internal = ADD_int*math.ceil(float(add_internal)/float(N))

  #RE
  sub_sw  = 0.0
  sub_int = 0.0
  sub_lk  = 0.0
  add_internal = ADD_int*max_add

  avg_mul_power = avg_mul_power/N
  avg_add_power= avg_add_power/N
  avg_index_power= avg_index_power/N

  xor_sw = 0.0
  xor_int = 0.0
  xor_lk = 0.0
  sh_sw  = 0.0
  sh_int = 0.0
  sh_lk  = 0.0

  #This is because the verilog counts regs we don't...
  buff = 0.0
  avg_reg_power= (avg_reg_power / N) + buff 
  avg_add_power = ADD_switch * max_add
  switch_compute = avg_mul_power + avg_add_power + avg_index_power + xor_sw + sub_sw + sh_sw
  internal_compute = mul_internal + add_internal + index_internal + xor_int + sub_int + sh_int
  total_power = switch_compute + avg_reg_power + \
              leakage_compute + reg_leakage + \
              internal_compute + reg_internal

  avg_total_power = total_power
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

  #f = open(os.path.join(BaseFile,'results', config_file), 'w+')
  #RES = [
         #"Area = %s, " % str(AREA),
         #"Power = %s, " % str(avg_total_power),
         #"Cycles = %s, " % str(N)]
  #for r in RES:
      #f.write(r)
  #f.close()

  return avg_total_power, AREA, N

if __name__ == '__main__':
  if(len(sys.argv) != 3 ):
    sys.exit(0)
  kernel = sys.argv[1]
  config_file = sys.argv[2]
  main(kernel, config_file)


