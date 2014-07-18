#!/usr/bin/env python
import os
import sys
import os.path
import time

import llvm_compile
import config

def main(kernel, part, unroll):

  part = int(part)
  unroll = int(unroll)

  if not 'ALADDIN_HOME' in os.environ:
    raise Exception('Set ALADDIN_HOME directory as an environment variable')
  
  if not 'TRACER_HOME' in os.environ:
    raise Exception('Set TRACER_HOME directory as an environment variable')
  

  ALADDIN_HOME = os.getenv('ALADDIN_HOME')
  BENCH_HOME = ALADDIN_HOME + '/SHOC/' + kernel

  os.chdir(BENCH_HOME)
  d = 'p%i_u%i' % (part, unroll)

  #Run LLVM-Tracer to generate the dynamic trace
  llvm_compile.main(BENCH_HOME, kernel)
  
  #Generate accelerator design config file
  config.main(BENCH_HOME, kernel, part, unroll)

  print 'Start Aladdin'
  trace_file = BENCH_HOME+ '/' + 'dynamic_trace'
  config_file = 'config_' + d
  
  newdir = os.path.join(BENCH_HOME, 'sim', d)
  print 'Changing directory to %s' % newdir
  
  os.chdir(newdir)
  os.system('%s/aladdin %s %s %s ' % (os.getenv('ALADDIN_HOME'), kernel, trace_file, config_file)) 

if __name__ == '__main__':
  kernel = sys.argv[1]
  part = sys.argv[2]
  unroll = sys.argv[3]
  main(kernel, part, unroll)

