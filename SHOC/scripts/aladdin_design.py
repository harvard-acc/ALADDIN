#!/usr/bin/python
import os
import sys
import subprocess
import shlex

def main(cil, trace, config):

  if not 'ALADDIN_HOME' in os.environ:
    print 'Set ALADDIN_HOME directory as an environment variable'
    sys.exit(0)

  print 'Running aladdin_design.main(), calling Aladdin'
    
  aladdin_callstring = '%s/aladdin %s %s %s' % (os.getenv('ALADDIN_HOME'), cil, trace, config)
  # print '>> %s' % aladdin_callstring

  out = open('aladdin.out', 'w')
  subprocess.call(shlex.split(aladdin_callstring), stdout=out, stderr=subprocess.STDOUT)
    # subprocess.Popen(shlex.split(aladdin_callstring), stdout=out, stderr=subprocess.STDOUT).communicate()
  #os.system('%s/aladdin %s %s %s' % (os.getenv('ALADDIN_HOME'), cil, trace, config )) 
  
  BENCH_HOME = os.getenv('ALADDIN_HOME') + '/SHOC/'
  #sys.path.append(os.getenv('BENCH_HOME'))
  #import parse_power_per_run_ss
  #return parse_power_per_run_ss.main(cil.split('.')[0], config2)
  #import internal_power_script
  #return internal_power_script.main(cil.split('.')[0], config2)
  # os.system('python %s/parse_power_per_run_ss.py n  %s' % (os.getenv('BENCH_HOME'), config2) )


  # curr_path = os.getcwd()
  #os.system('rm -r ' + curr_path)
  #os.system('rm 20')
  #def aladdin_call(cil, trace, config, ):

if __name__ == '__main__':
  cil = sys.argv[1]
  trace = sys.argv[2]
  config = sys.argv[3]
  main(cil, trace, config)
