#!/group/brooks/shao/python-System/install/bin/python
import os
import sys
import os.path
import subprocess
import time

#import compile
import llvm_compile
import config
import pre_processing
import aladdin_design

def main(kernel, part, unroll):

  part = int(part)
  unroll = int(unroll)

  if not 'ALADDIN_HOME' in os.environ:
    raise Exception('Set ALADDIN_HOME directory as an environment variable')

  ALADDIN_HOME = os.getenv('ALADDIN_HOME')
  BENCH_HOME = ALADDIN_HOME + '/SHOC/'

  BMKROOT = BENCH_HOME  + '/' + kernel
  os.chdir(BMKROOT)

  d = 'p%i_u%i' % (part, unroll)

  llvm_compile.main(BENCH_HOME + "/" + kernel, kernel)
  
  config.main(kernel, part, unroll)

  print 'Continuing run_aladdin_local'

  #OLD_LIB = os.getenv('LD_LIBRARY_PATH')
  #os.environ['LD_LIBRARY_PATH'] = ALADDIN_HOME + '/lib:' +  OLD_LIB
  bench_name = kernel

  trace_file = BMKROOT + '/' + 'dynamic_trace'

  config_file = 'config_' + d

  newdir = os.path.join(BMKROOT, 'sim', d)
  print 'Changing directory to %s' % newdir
  os.chdir(newdir)

  # This is what we would call for each (p,u pair)
  return aladdin_design.main(bench_name, trace_file, config_file)

  # return power, area, cycles

if __name__ == '__main__':
  kernel = sys.argv[1]
  part = sys.argv[2]
  unroll = sys.argv[3]
  main(kernel, part, unroll)

