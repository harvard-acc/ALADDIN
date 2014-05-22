#!/usr/bin/env python 
import os
import sys

if not 'BENCH_HOME' in os.environ:
  print 'Set BENCH_HOME directory as an environment variable'
  sys.exit(0)

if not 'ALADDIN_HOME' in os.environ:
  print 'Set ALADDIN_HOME directory as an environment variable'
  sys.exit(0)

BENCH_HOME = os.getenv('BENCH_HOME')
ALADDIN_HOME = os.getenv('ALADDIN_HOME')
kernel = sys.argv[1]

BMKROOT = BENCH_HOME 
os.chdir(BMKROOT)
os.system('python ' + ALADDIN_HOME + '/scripts/compile.py ' + kernel)
os.system('python ' + ALADDIN_HOME + '/scripts/nnet_config.py ' + kernel)
os.system('python ' + ALADDIN_HOME + '/scripts/pre_processing.py ' + kernel)

bench_name = 'nnet.cil'
trace_file = BMKROOT + '/' + bench_name + '_dump.gz'
addr_file = BMKROOT + '/' + bench_name + '_base_addr'
OLD_LIB = os.getenv('LD_LIBRARY_PATH')
os.environ['LD_LIBRARY_PATH'] = ALADDIN_HOME + '/lib:' +  OLD_LIB
dirs = os.listdir(BMKROOT + '/sim/')
for d in dirs:
  os.chdir(BMKROOT + '/sim/' + d)
  config_file = d + '.config'
  os.system(ALADDIN_HOME + '/aladdin ' + bench_name + ' ' + trace_file + ' ' +\
    config_file + ' ' + addr_file)
