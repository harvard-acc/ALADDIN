#!/usr/bin/env python
import os
import sys

def main(binary, trace, config):

  if not 'ALADDIN_HOME' in os.environ:
    print 'Set ALADDIN_HOME directory as an environment variable'
    sys.exit(0)

  print 'Running aladdin_design.main(), calling Aladdin'
  aladdin_callstring = '%s/aladdin %s %s %s' % (os.getenv('ALADDIN_HOME'), binary, trace, config)
  os.system('%s/aladdin %s %s %s 2> aladdin.out' % (os.getenv('ALADDIN_HOME'), binary, trace, config )) 

if __name__ == '__main__':
  binary = sys.argv[1]
  trace = sys.argv[2]
  config = sys.argv[3]
  main(binary, trace, config)
