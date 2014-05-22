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


array_names = {
'sig' :
['data','weights','num_units','num_rows','num_columns','hid','hid_temp','sigmoid_table'],
'relu' :
['data','weights','num_units','num_rows','num_columns','hid','hid_temp'],
}
array_partition_type = {
'data' : 'cyclic', 
'weights' : 'cyclic',
'num_units' : 'complete',
'num_rows' : 'complete',
'num_columns' : 'complete',
'hid' : 'cyclic',
'hid_temp' : 'cyclic',
'sigmoid_table' : 'cyclic',
}

#to get NUM_LAYER
#data = NUM_TEST_CASES * INPUT_DIM
#weights  = w_size = sum(INPUT_DIM + 1 << LOG_NUM_UNITS[i] + NUM_CLASSES)
#hid/hid_temp = NUM_TEST_CASES * biggest( 1<< LOG_NUM_UNITS[i], NUM_CLASSES)
#num_units = NUM_LAYERS + 2
#num_rows = NUM_LAYERS + 1
#num_columns = NUM_LAYERS + 1
#sigmoid_table = 1<< LOG_SIGMOID_COARSENESS

NUM_TEST_CASES = 0
INPUT_DIM = 0
LOG_NUM_UNITS = []
NUM_CLASSES = 0
NUM_LAYERS = 0
ACTIVATION_FUN = 0
LOG_SIGMOID_COARSENESS = 0

header = open('nnet_fwd.h', 'r')
log_flag = 0
for line in header:
  if log_flag == 0:
    if 'NUM_CLASSES' in line and 'define' in line:
      parts = line.rstrip('\n').split(' ')
      NUM_CLASSES = int(parts[2])
      
    elif 'INPUT_DIM' in line and 'define' in line:
      parts = line.rstrip('\n').split(' ')
      INPUT_DIM = int(parts[2])
      
    elif 'NUM_TEST_CASES' in line and 'define' in line:
      parts = line.rstrip('\n').split(' ')
      NUM_TEST_CASES = int(parts[2])
      
    elif 'NUM_LAYERS' in line and 'define' in line:
      parts = line.rstrip('\n').split(' ')
      NUM_LAYERS = int(parts[2])
    elif 'LOG_NUM_UNITS' in line:
      log_flag = 1
    elif 'ACTIVATION_FUN' in line and 'define' in line:
      parts = line.rstrip('\n').split(' ')
      ACTIVATION_FUN = int(parts[2])
    elif 'LOG_SIGMOID_COARSENESS' in line and 'define' in line:
      parts = line.rstrip('\n').split(' ')
      LOG_SIGMOID_COARSENESS = int(parts[2])
    else:
      continue
  else:
    parts = line.rstrip('\n').strip().split(',')
    print parts
    for item in parts:
      LOG_NUM_UNITS.append(int(item))
    log_flag = 0
header.close()

array_size = {}
#data = NUM_TEST_CASES * INPUT_DIM
#weights  = w_size = sum(INPUT_DIM + 1 << LOG_NUM_UNITS[i] + NUM_CLASSES)
#hid/hid_temp = NUM_TEST_CASES * biggest( 1<< LOG_NUM_UNITS[i], NUM_CLASSES)
#num_units = NUM_LAYERS + 2
#num_rows = NUM_LAYERS + 1
#num_columns = NUM_LAYERS + 1
#sigmoid_table = 1<< LOG_SIGMOID_COARSENESS

array_size['data'] =  NUM_TEST_CASES * INPUT_DIM
array_size['weights'] = INPUT_DIM
for value in LOG_NUM_UNITS:
  array_size['weights'] += 1 << value

array_size['weights'] += NUM_CLASSES

biggest_row = 0
for value in LOG_NUM_UNITS:
  if biggest_row < (1 << value):
    biggest_row = 1 << value
if biggest_row < NUM_CLASSES:
  biggest_row = NUM_CLASSES
array_size['hid'] = biggest_row * NUM_TEST_CASES
array_size['hid_temp'] = biggest_row * NUM_TEST_CASES
array_size['num_units'] = NUM_LAYERS + 2
array_size['num_rows'] = NUM_LAYERS + 1
array_size['num_columns'] = NUM_LAYERS + 1
array_size['sigmoid_table'] = 1 << LOG_SIGMOID_COARSENESS


if ACTIVATION_FUN == 0:
  mode = 'relu'
else:
  mode = 'sig'
print mode
unrolling       = [1, 2, 4, 8, 16, 32]
partition = [1, 2, 4, 8, 16, 32]
print ACTIVATION_FUN
os.system('mkdir ' + BaseFile + '/sim/')
for unroll in unrolling:
  for part in partition:
    d = 'p%i_u%i' % (part, unroll)
    print d
    os.system('mkdir ' + BaseFile + '/sim/' + d)
    config = open( BaseFile + '/sim/' + d + '/config_' + d, 'w')
    print array_names[mode]
    for key in array_names[mode]:
      print key
      value = array_partition_type[key]
      print key, value
      if value == 'complete':
        config.write(key + ',' + value + ',' + str(array_size[key]) + "\n")
      elif value == 'cyclic' or value == 'block':
        config.write(key + ',' + value + ',' + str(array_size[key]) + ',' + str(part) + "\n")
      else:
        print "What else???"
        sys.exit(0)
    config.write('clear_matrix,unrolling,' + str(part) + "\n")
    config.write('copy_matrix,unrolling,' + str(part) + "\n")
    config.write('RELU,unrolling,' + str(part) + "\n")
    config.write('matrix_multiply_kernel,unrolling,' + str(unroll) + "\n")
    config.write('grab_matrix,unrolling,' + str(NUM_LAYERS) + "\n")
    config.write('matrix_multiply,unrolling,' + str(1) + "\n")
    config.close()


'''

a = 1
for p in partition:
  for i in i_unrolling:
    print p, i, a
    d = 'p%i_i%i_a%i' % (p, i, a)
    os.system('mkdir p%i_i%i_a%i' % (p, i, a))
    os.system('cp ../../NewMicroBenchmarks/' + bench + '/' + d + '/' + \
      bench + '_total_level.gz ./' + d)
    config_file = open("./p%i_i%i_a%i/config_p%i_i%i_a%i"\
                             % (p, i, a, p, i, a ), 'w')
    for addr in complete:
      config_file.write('complete,'+ str(addr) +'\n')
    
    k = 0
    for addr in baseaddress:
      config_file.write('partition,'+ str(addr) +',' \
        + partition_type[k] + ',' + str(p) + '\n')
      k+=1
    #k = 0
    #for addr in part4:
      #config_file.write('partition,'+ str(addr) +',' \
        #+ partition_type[k] + ',' + str(8) + '\n')
      #k+=1
    
    for key in flatten_inst:
      tmp_list = flatten_inst[key]
      for l in tmp_list:
        config_file.write('flatten,' + str(method[key]) + ','\
           + str(l) + '\n')
    for key in unrolling_inst:
      tmp_list = unrolling_inst[key]
      for l in tmp_list:
        config_file.write('unrolling,' + str(method[key]) + ','\
         + str(l) + ',' + str(i) + '\n')
      
    #memory ambiguation
    config_file.write('ambiguation,' + str(a) + '\n')
    config_file.close()
'''
