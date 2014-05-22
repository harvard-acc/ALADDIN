#!/usr/bin/env python
import numpy as np
import sys
import os
import glob

#bench = sys.argv[1]

BENCH_HOME = os.getenv('BENCH_HOME')
BaseFile = BENCH_HOME
os.chdir(BaseFile)
#print "changing to directory: " + BaseFile + bench 
kernel = 'nnet'
BINARY=kernel  + '.cil'

base_mem_id = {
0: 'data', 
1: 'weights', 
2: 'num_units',
3: 'num_rows', 
4: 'num_columns', 
5: 'hid',
6: 'hid_temp',
7: 'sigmoid_table',
}

#read loop increment instruction
loop_inc = {}
ind_var = open(BINARY + '_induction_variables', 'r')
for line in ind_var:
  parts = line.rstrip('\n').split(',')
  if parts[2] == parts[3]: #base induction variables
    #print line, parts
    if parts[0] in loop_inc: 
      tmp = loop_inc[parts[0]]
      tmp.append(int(parts[4]))
      loop_inc[parts[0]] = tmp
    else:
      tmp = []
      tmp.append(int(parts[4]))
      loop_inc[parts[0]] = tmp

print loop_inc

mem_addr = {}
base_addr = open(BINARY + '_base_addr', 'r')
for line in base_addr:
  parts = line.rstrip('\n').split(',')
  addr_id = int(parts[1])
  addr = parts[4]
  if addr_id not in base_mem_id:
    continue
  mem_addr[base_mem_id[addr_id]] = addr
base_addr.close()
print mem_addr

method = {}
method_map = open(BINARY + '_method_map', 'r')
for line in method_map:
  parts = line.rstrip('\n').split(',')
  method[parts[2]] = parts[1]
method_map.close()
print method

dirs = os.listdir(BaseFile + '/sim/')
for d in dirs:
  config = open(BaseFile + '/sim/' + d + '/config_' + d, 'r')
  new_config = open(BaseFile + '/sim/' + d + '/' + d + '.config' , 'w')
  for line in config:
    parts = line.rstrip('\n').split(',')
    if parts[0] in mem_addr:
      new_config.write('partition,' + str(mem_addr[parts[0]]) + ',' )
      i = 1
      while i < len(parts):
        new_config.write(parts[i] + ',')
        i += 1
      new_config.write("\n")
    elif parts[0] in loop_inc:
      for item in loop_inc[parts[0]]:
        new_config.write(str(parts[1]) + ',')
        new_config.write(str(method[parts[0]]) + ',')
        new_config.write(str(item) + ',')
        new_config.write(str(parts[2]) + ',')
        new_config.write("\n")
    else:
      print line
  
  config.close()
  new_config.close()
  os.system('cp ' + BaseFile + '/' + BINARY + '_induction_variables ' + BaseFile + '/sim/' + d + '/')

