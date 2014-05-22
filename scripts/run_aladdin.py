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


OLD_LIB = os.getenv('LD_LIBRARY_PATH')
os.environ['LD_LIBRARY_PATH'] = ALADDIN_HOME + '/lib:' +  OLD_LIB
#dispatch condor jobs
con_name = BMKROOT + '/' + bench_name + '.con'
condor_file.open(con_name)
basicCondor = ['#Submit this file in the directory where the input and output files live',
		'Universe        = vanilla',
		'# With the following setting, Condor will try to copy and use the environ-',
		'# ment of the submitter, provided its not too large.',
		'GetEnv          = True',
		'# This forces the jobs to be run on the less crowded server',
		'Requirements    = (OpSys == "LINUX") && (Arch == "X86_64")',
		'# Change the email address to suit yourself',
		'Notify_User     = shao@eecs.harvard.edu',
		'Notification    = Error',]
for line in baseCondor:
  condor_file.write( line + '\n');

bench_name = 'nnet.cil'
trace_file = BMKROOT + '/' + bench_name + '_dump.gz'
addr_file = BMKROOT + '/' + bench_name + '_base_addr'


dirs = os.listdir(BMKROOT + '/sim/')
for d in dirs:
  config_file = d + '.config'
  #os.chdir(BMKROOT + '/sim/' + d)
  #os.system(ALADDIN_HOME + '/aladdin ' + bench_name + ' ' + trace_file + ' ' +\
    #config_file + ' ' + addr_file)
  condor_file.write(BMKROOT + '/sim/' + d + ' \n')
  args = 'Arguments = ' + bench_name + ' ' + trace_file + ' ' + \
    config_file + ' ' +  addr_file)
  condor_file.write(args + ' \n')
  condor_file.write('Output = ' + d + '.aladdin.out \n')
  condor_file.write('Error = ' + d + '.aladdin.err \n')
  condor_file.write('Queue \n')

condor_file.close()

os.system('condor_submit ' + con_name)
