#!/usr/bin/env python
import sys
import os
import shutil


def main(directory, kernel, part, unroll, pipe):

  print '--Running config.main()'
  
  d = 'p%s_u%s_P%s' % (part, unroll, pipe)

  print 'Kernel = %s, Part = %s, unroll = %s' % (kernel, part, unroll)

  array_names = {
  'bb_gemm' : ['x','y','z'],
  'fft' : ['work_x','work_y','DATA_x','DATA_y','data_x','data_y','smem','reversed','sin_64','cos_64','sin_512','cos_512'],
  'md' : ['position_x','position_y','position_z','NL'],
  'pp_scan' : ['bucket','bucket2','sum'],
  'reduction' : ['in'],
  'ss_sort' : ['a','b','bucket','sum'],
  'stencil' : ['orig','sol','filter'],
  'triad' : ['a','b','c'],
  }
  array_partition_type = {
  'bb_gemm' : ['cyclic','cyclic','cyclic'],
  'fft' : ['cyclic','cyclic','cyclic','cyclic','complete','complete','cyclic','complete','complete','complete','complete','complete'],
  'md' : ['cyclic','cyclic','cyclic','cyclic'],
  'pp_scan' : ['cyclic','cyclic','cyclic'],
  'reduction' : ['cyclic'],
  'ss_sort' : ['cyclic','cyclic','cyclic','cyclic'],
  'stencil' : ['cyclic','cyclic','complete'],
  'triad' : ['cyclic','cyclic','cyclic'],
  }
  array_size = {
  'bb_gemm' : ['1024','1024','1024'],
  'fft' : ['512','512','512','512','8','8','576','8','448','448','448','448'],
  'md' : ['32','32','32','1024'],
  'pp_scan' : ['2048','2048','16'],
  'reduction' : ['2048'],
  'ss_sort' : ['2048','2048','2048','128'],
  'stencil' : ['1156','1156','9'],
  'triad' : ['2048','2048','2048'],
  }

  BaseFile = directory
  os.chdir(BaseFile)
  
  if not os.path.isdir(BaseFile + '/sim/'):
    os.mkdir(BaseFile + '/sim/')
  
  if os.path.isdir(BaseFile + '/sim/' + d):
    shutil.rmtree(BaseFile + '/sim/' + d)

  if not os.path.isdir(BaseFile + '/sim/' + d):
    os.mkdir(BaseFile + '/sim/' + d)

  print 'Writing config file'
  config = open(BaseFile + '/sim/' + d + '/config_' + d, 'w')
  config.write('pipelining,' + str(pipe) + "\n")
  #memory partition
  names = array_names[kernel]
  types = array_partition_type[kernel]
  sizes = array_size[kernel]
  assert (len(names) == len(types) and len(names) == len(sizes))
  for name,type,size in zip(names, types, sizes):
    if type == 'complete':
      config.write('partition,'+ type + ',' + name + ',' + str(size) + "\n")
    elif type == 'block' or type == 'cyclic':
      config.write('partition,'+ type + ',' + name + ',' + str(size) + ',' + str(part) + "\n")
    else:
      print "Unknown partition type: " + type 
      sys.exit(0)
  #loop unrolling and flattening
  if kernel == 'bb_gemm': 
    config.write('unrolling,bb_gemm,5,%s\n' %(unroll))
    config.write('flatten,bb_gemm,6\n' )
    config.write('flatten,bb_gemm,8\n' )

  elif kernel == 'fft':
    config.write('unrolling,step1,16,%s\n' %(unroll))
    config.write('flatten,step1,18\n')
    config.write('flatten,step1,26\n')
    config.write('flatten,step1,36\n')
    config.write('unrolling,step2,53,%s\n' %(unroll))
    config.write('flatten,step2,54\n')
    config.write('unrolling,step3,75,%s\n' %(unroll))
    config.write('flatten,step3,76\n')
    config.write('flatten,step3,83\n')
    config.write('unrolling,step4,100,%s\n' %(unroll))
    config.write('flatten,step4,101\n')
    config.write('unrolling,step5,122,%s\n' %(unroll))
    config.write('flatten,step5,123\n')
    config.write('flatten,step5,130\n')
    config.write('unrolling,step6,149,%s\n' %(unroll))
    config.write('flatten,step6,151\n')
    config.write('flatten,step6,164\n')
    config.write('flatten,step6,174\n')
    config.write('unrolling,step7,193,%s\n' %(unroll))
    config.write('flatten,step7,194\n')
    config.write('unrolling,step8,216,%s\n' %(unroll))
    config.write('flatten,step8,217\n')
    config.write('flatten,step8,224\n')
    config.write('unrolling,step9,243,%s\n' %(unroll))
    config.write('flatten,step9,244\n')
    config.write('unrolling,step10,266,%s\n' %(unroll))
    config.write('flatten,step10,267\n')
    config.write('flatten,step10,274\n')
    config.write('unrolling,step11,293,%s\n' %(unroll))
    config.write('flatten,step11,295\n')
    config.write('flatten,step11,304\n')
    
  elif kernel == 'md':
    config.write('unrolling,md_kernel,19,%s\n' %(unroll))
    config.write('partition,cyclic,d_force_x,32,1\n')
    config.write('partition,cyclic,d_force_y,32,1\n')
    config.write('partition,cyclic,d_force_z,32,1\n')

  elif kernel == 'pp_scan':
    config.write('unrolling,sum_scan,26,%s\n' %(unroll))
    config.write('unrolling,local_scan,15,%s\n' %(unroll))
    config.write('flatten,local_scan,16\n')
    config.write('unrolling,last_step_scan,33,%s\n' %(unroll))
    config.write('flatten,last_step_scan,34\n')

  elif kernel == 'reduction':
    config.write('unrolling,reduction,8,%s\n' %(unroll))
  
  elif kernel == 'ss_sort':
    config.write('unrolling,init,52,%s\n' %(part))
    config.write('unrolling,hist,61,%s\n' %(unroll))
    config.write('flatten,hist,63,%s\n' %(unroll))
    config.write('unrolling,local_scan,17,%s\n' %(unroll))
    config.write('flatten,local_scan,19,%s\n' %(unroll))
    config.write('unrolling,sum_scan,30,%s\n' %(unroll))
    config.write('unrolling,last_step_scan,38,%s\n' %(unroll))
    config.write('flatten,last_step_scan,40,%s\n' %(unroll))
    config.write('unrolling,update,75,%s\n' % (unroll))
    config.write('flatten,update,77,%s\n' %(unroll))

  elif kernel == 'stencil':
    config.write('unrolling,stencil,11,%s\n' %(unroll))
    config.write('flatten,stencil,12,%s\n' %(unroll))

  #elif kernel == 'triad':
    #config.write('unrolling,triad,5,%s\n' %(unroll))

  config.close()

if __name__ == '__main__':
  directory = sys.argv[1]
  kernel = sys.argv[2]
  part = sys.argv[3]
  unroll = sys.argv[4]
  pipe = sys.argv[5]
  main(directory, kernel, part, unroll, pipe)
