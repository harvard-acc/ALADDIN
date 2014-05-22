#!/usr/bin/env python 
import os
import sys
import operator
import gzip
import math
from collections import defaultdict

sources_dict = {
'nnet' : 'nnet_fwd.c',
}

kernels = {
'nnet-0' :
'nnet_fwd_relu,matrix_multiply,matrix_multiply_kernel,RELU,clear_matrix,copy_matrix,grab_matrix',
'nnet-1' :
'nnet_fwd_sig,matrix_multiply,matrix_multiply_kernel,sigmoid_lookup,clear_matrix,copy_matrix,grab_matrix',
}

top_func = {
'nnet-0' : 'nnet_fwd_relu',
'nnet-1' : 'nnet_fwd_sig',
}
def compile (kernel):
  
  SOURCE_FILES=sources_dict[kernel]
  BINARY=kernel  + '.cil'
  header = open('nnet_fwd.h', 'r')
  for line in header:
    if 'ACTIVATION_FUN' in line and 'define' in line:
      parts = line.rstrip('\n').split(' ')
      ACTIVATION_FUN = parts[2]
      break
  header.close()
  os.environ['WORKLOAD']=kernels[kernel + '-' + ACTIVATION_FUN]
  os.environ['TOPFUNC'] = top_func[kernel + '-' + ACTIVATION_FUN]
  print "==========Compiling..."
  os.environ['ILDJIT_PATH']='/group/brooks/ildjit/libraries/cscc/lib:/group/brooks/ildjit/libraries/gcc4cli/lib'
  os.environ['ILDJIT_HOME'] = '/group/brooks/ildjit/release/'
  ILDJIT_HOME = os.getenv('ILDJIT_HOME')
  OLD_PATH = os.getenv('PATH')
  os.environ['PATH'] = ILDJIT_HOME + '/bin:' + OLD_PATH + ':/group/brooks/xan/Softwares/GCC/GCC4NET/image/bin'
  OLD_LIB = os.getenv('LD_LIBRARY_PATH')
  os.environ['LD_LIBRARY_PATH'] ='/group/brooks/xan/System/lib:' + OLD_LIB + ':' + ILDJIT_HOME + '/lib:' + ILDJIT_HOME + '/lib/iljit'

  os.environ['ILDJIT_CUSTOM_OPTIMIZATION'] = '0'
  COMPILER='cil32-gcc'
  os.chdir(BMKROOT)
  print COMPILER + ' ' + SOURCE_FILES + ' -lm -o ' + BINARY
  os.system(COMPILER + ' ' + SOURCE_FILES + ' -lm -o ' + BINARY)

  HOME = os.getenv('HOME')
  if os.path.exists(HOME + '/.ildjit/manfred/' + BINARY):
    os.system('rm -r ' + HOME + '/.ildjit/manfred/' + BINARY)
    #print 'rm -r ' + HOME + '/.ildjit/manfred/' + BINARY

  PROGRAM= 'iljit'
  ILJITARGS='-P1'
  ARGS= BMKROOT + '/' + BINARY + ' input.data check.data'
  OUTPUT = ' 1> ' + kernel + '.out 2> ' + kernel + '.err'
  os.system( PROGRAM + ' ' + ILJITARGS + ' ' + ARGS + ' ' + OUTPUT)
  #print PROGRAM + ' ' + ILJITARGS + ' ' + ARGS + ' ' + OUTPUT

  ILJITARGS="-P1 -O3 -N  -M  -R  --static --disable-optimizations=methodinline "
  os.system( PROGRAM + ' ' + ILJITARGS + ' ' + ARGS + ' ' + OUTPUT)
  #print PROGRAM + ' ' + ILJITARGS + ' ' + ARGS + ' ' + OUTPUT

  ILJITARGS="-O3 -N  -M  -R  --static --disable-optimizations=methodinline "
  os.system( PROGRAM + ' ' + ILJITARGS + ' ' + ARGS + ' ' + OUTPUT)
  #print PROGRAM + ' ' + ILJITARGS + ' ' + ARGS + ' ' + OUTPUT

  #1. profile dynamic trace

  print "==========Profiling Dynamic Trace..."
  os.environ['ILDJIT_CUSTOM_OPTIMIZATION'] = '1'
  os.environ['ILDJIT_PLUGINS'] ='/group/vlsiarch/shao/Projects/Release/alibaba/iljit-plugins/lib/lib/iljit/optimizers/wholetrace'
  PROGRAM= 'iljit'
  ILJITARGS="-O3 -N  -M  -R  --static --disable-optimizations=methodinline "
  ARGS= BMKROOT + '/' + BINARY + ' input.data check.data'
  OUTPUT = ' 1> ' + kernel + '.out 2> ' + kernel + '.err'
  os.chdir(BMKROOT)
  #os.system( PROGRAM + ' ' + ILJITARGS + ' ' + ARGS)
  os.system( PROGRAM + ' ' + ILJITARGS + ' ' + ARGS + ' ' + OUTPUT)
  
if not 'BENCH_HOME' in os.environ:
  print 'Set BENCH_HOME directory as an environment variable'
  sys.exit(0)

BENCH_HOME = os.getenv('BENCH_HOME')
kernel = sys.argv[1]


BMKROOT = BENCH_HOME 
os.chdir(BMKROOT)
compile(kernel)
