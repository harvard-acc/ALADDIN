#!/usr/bin/env python
import sys
import os
import shutil

def sizeinbytes (type):
  if type == 'uint8' or type == 'char':
    return 1
  elif type == 'int' or type == 'float':
    return 4
  elif type == 'double' or type == 'uint64':
    return 8
    
def main(directory, kernel, algorithm, part, unroll, pipe, cycle_time):

  print '--Running config.main()'

  d = 'p%s_u%s_P%s_%sns' % (part, unroll, pipe, cycle_time)

  print 'Kernel = %s, Part = %s, unroll = %s Pipeline = %s'  % (kernel, part, unroll, pipe)

  array_names = {
   'aes-aes' : ['ctx','k','buf','rcon','sbox'],
   'backprop-backprop' : ['weights','inputs','targets','changeMat','activations','deltas','layer_size'],
   'bfs-bulk' : ['nodes','edges','level','level_counts'],
   'bfs-queue' : ['queue','nodes','edges','level','level_counts'],
   'fft-strided' : ['real','img','real_twid','img_twid'],
   'fft-transpose':['reversed','DATA_x','DATA_y','data_x','data_y','smem','work_x','work_y'],
   'gemm-blocked':['m1','m2','prod'],
   'gemm-ncubed' :['m1','m2','prod'],
   'kmp-kmp' :['pattern','input','kmpNext'],
   'md-knn':['d_force_x','d_force_y','d_force_z','position_x','position_y','position_z','NL'],
   'md-grid' : ['n_points','d_force','position'],
   'nw-nw' : ['SEQA','SEQB','allignedA','allignedB','A','ptr'],
   'sort-merge' : ['temp','a'],
   'sort-radix' :['a','b','bucket','sum'],
   'spmv-crs' :['val','cols','rowDelimiters','vec','out'],
   'spmv-ellpack' :['nzval','cols','vec','out'],
   'stencil-stencil2d' :['orig','sol','filter'],
   'stencil-stencil3d' : ['orig','sol'],
   'viterbi-viterbi' : ['Obs','transMat','obsLik','v'],
  }
  array_partition_type = {
   'aes-aes' : ['cyclic','cyclic','cyclic','complete','cyclic'],
   'backprop-backprop' : ['cyclic','cyclic','cyclic','cyclic','cyclic','cyclic','cyclic'],
   'bfs-bulk'  :['cyclic','cyclic','cyclic','cyclic'],
   'bfs-queue' :['cyclic','cyclic','cyclic','cyclic','cyclic'],
   'fft-strided'  :['cyclic','cyclic','cyclic','cyclic'],
   'fft-transpose' :['complete','cyclic','cyclic','complete','complete','cyclic','cyclic','cyclic'],
   'gemm-blocked':['cyclic','cyclic','cyclic'],
   'gemm-ncubed' :['cyclic','cyclic','cyclic'],
   'kmp-kmp'  :['cyclic' ,'cyclic','cyclic'],
   'md-knn' :['cyclic','cyclic','cyclic','cyclic','cyclic','cyclic','cyclic'],
   'md-grid' :['cyclic','cyclic','cyclic'],
   'nw-nw' :['cyclic','cyclic','cyclic','cyclic','cyclic','cyclic'],
   'sort-merge' :['cyclic','cyclic'],
   'sort-radix' :['cyclic','cyclic','cyclic','cyclic'],
   'spmv-crs' :['cyclic','cyclic','cyclic','cyclic','cyclic'],
   'spmv-ellpack' :['cyclic','cyclic','cyclic','cyclic'],
   'stencil-stencil2d' :['cyclic','cyclic','complete'],
   'stencil-stencil3d' :['cyclic','cyclic'],
   'viterbi-viterbi' :['cyclic','cyclic','cyclic','cyclic'],
  }
  array_size = {
   'aes-aes' : ['96','32','16','1','256'],
   'backprop-backprop' : ['200','20','50','300','30','30','3'],
   'bfs-bulk'  :['512','4096','256','10'],
   'bfs-queue' :['256','512','4096','256','10'],
   'fft-strided'  :['1024','1024','1024','1024'],
   'fft-transpose' :['8','512','512','8','8','576','512','512'],
   'gemm-blocked' :['4096','4096','4096'],
   'gemm-ncubed' :['4096','4096','4096'],
   'kmp-kmp'  :['4','32411','4'],
   'md-knn' :['256','256','256','256','256','256','4096'],
   'md-grid' :['64','1920','1920'],
   'nw-nw' :['128','128','256','256','16641','16641'],
   'sort-merge' :['4096','4096'],
   'sort-radix' :['2048','2048','2048','128'],
   'spmv-crs' :['1666','1666','495','494','494'],
   'spmv-ellpack' :['4940','4940','494','494'],
   'stencil-stencil2d' :['8192','8192','9'],
   'stencil-stencil3d' :['16384','16384'],
   'viterbi-viterbi' :['128','4096','4096','4096']
  }
  #in bytes
  array_wordsize = {
   'aes-aes' : ['uint8','uint8','uint8','uint8','uint8'],
   'backprop-backprop' : ['double','double','double','double','double','double','int'],
   'bfs-bulk' : ['uint64','uint64','uint8','uint64'],
   'bfs-queue' : ['uint64','uint64','uint64','uint8','uint64'],
   'fft-strided' : ['double','double','double','double'],
   'fft-transpose':['int','double','double','double','double','double','double','double'],
   'gemm-blocked':['int','int','int'],
   'gemm-ncubed' :['int','int','int'],
   'kmp-kmp' :['char','char','int'],
   'md-knn':['double','double','double','double','double','double','double'],
   'md-grid' : ['int','double','double'],
   'nw-nw' : ['char','char','char','char','int','char'],
   'sort-merge' : ['int','int'],
   'sort-radix' :['int','int','int','int'],
   'spmv-crs' :['double','int','int','double','double'],
   'spmv-ellpack' :['double','int','double','double'],
   'stencil-stencil2d' :['int','int','int'],
   'stencil-stencil3d' : ['int','int'],
   'viterbi-viterbi' : ['int','float','float','float'],
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
  config.write('cycle_time,' + cycle_time + "\n")
  print "CYCLE_TIME," + cycle_time
  config.write('pipelining,' + str(pipe) + "\n")
  if algorithm == '':
    bench = kernel
  else:
    bench = kernel+'-'+algorithm
  #memory partition
  names = array_names[bench]
  types = array_partition_type[bench]
  sizes = array_size[bench]
  wordsizes = array_wordsize[bench]
  assert (len(names) == len(types) and len(names) == len(sizes))
  for name,type,size,wordtype in zip(names, types, sizes, wordsizes):
    wordsize = sizeinbytes(wordtype)
    if type == 'complete':
      config.write('partition,'+ type + ',' + name + ',' + \
      str(int(size)*int(wordsize)) + "\n")
    elif type == 'block' or type == 'cyclic':
      config.write('partition,'+ type + ',' + name + ',' + \
      str(int(size)*int(wordsize)) + ',' + str(wordsize) + ',' + str(part) + "\n")
    else:
      print "Unknown partition type: " + type
      sys.exit(0)

  #loop unrolling and flattening
  if bench == 'aes-aes':  # TODO need to fix
    config.write('flatten,gf_alog,73\n')
    config.write('flatten,gf_log,83\n')
    config.write('flatten,aes_subBytes,122\n') # trip_cnt = 16
    config.write('flatten,aes_addRoundKey,130\n') # trip_cnt = 16
    config.write('flatten,aes_addRoundKey_cpy,138\n') # trip_cnt = 16
    config.write('flatten,aes_mixColumns,159\n') # trip_cnt = 4
    config.write('flatten,aes_expandEncKey,179\n') # trip_cnt = 16
    config.write('flatten,aes_expandEncKey,186\n') # trip_cnt = 16
    config.write('unrolling,aes256_encrypt_ecb,203,%s\n' %(unroll)) #trip_cnt = 32
    config.write('unrolling,aes256_encrypt_ecb,206,%s\n' %(unroll)) #trip_cnt = 8
    config.write('unrolling,aes256_encrypt_ecb,212,%s\n' %(unroll)) #tc = 13
  elif bench == 'backprop-backprop':
    config.write('unrolling,update_layer,17,%s\n' %(unroll))
    config.write('flatten,update_layer,19\n')
    config.write('unrolling,update,32,%s\n' %(unroll))
    config.write('unrolling,update,37,%s\n' %(unroll))
    config.write('unrolling,propagate_error_out,48,%s\n' %(unroll))
    config.write('unrolling,propagate_error_layer,67,%s\n' %(unroll))
    config.write('flatten,propagate_error_layer,69\n')
    config.write('unrolling,update_weights,84,%s\n' %(unroll))
    config.write('flatten,update_weights,85\n')
    config.write('unrolling,propagate_errors,106,%s\n' %(unroll))
    config.write('flatten,propagate_errors,107\n')
    config.write('unrolling,propagate_errors,114,%s\n' %(unroll))
    config.write('unrolling,propagate_errors,119,%s\n' %(unroll))
    config.write('unrolling,comp_error,132,%s\n' %(unroll))
    config.write('unrolling,backprop,151,%s\n' %(unroll))
    config.write('flatten,backprop,152\n')
    config.write('flatten,backprop,154\n')
    config.write('unrolling,backprop,160,%s\n' %(unroll))
    config.write('flatten,backprop,162\n')
    config.write('flatten,backprop,163\n')
    config.write('flatten,backprop,164\n')
  elif bench == 'bfs-bulk':
    if unroll >= 10:
      config.write('flatten,bfs,53\n')
    else:
      config.write('unrolling,bfs,53,%s\n' %(unroll))               # trip_cnt = 10
    config.write('flatten,bfs,56\n')
    config.write('flatten,bfs,60\n')
  elif bench == 'bfs-queue':
    config.write('unrolling,bfs,63,%s\n' %(unroll))                 # trip_cnt = 256
    config.write('flatten,bfs,70\n')
  elif bench == 'fft-strided':
    if unroll >= 10:                                              # trip_cnt = 10
      config.write('flatten,fft,14\n')
    else:
      config.write('unrolling,fft,14,%s\n' %(unroll))
    config.write('flatten,fft,15\n')
  elif bench == 'fft-transpose':
    if unroll >= 8:
      config.write('flatten,twiddles8,54\n')
    else:
      config.write('unrolling,twiddles8,54,%s\n' %(unroll))       # trip_cnt = 8
    if unroll >=64:                                               # trip_cnt = 64
      config.write('flatten,fft1D_512,154\n')
      config.write('flatten,fft1D_512,201\n')
      config.write('flatten,fft1D_512,215\n')
      config.write('flatten,fft1D_512,231\n')
      config.write('flatten,fft1D_512,246\n')
      config.write('flatten,fft1D_512,271\n')
      config.write('flatten,fft1D_512,321\n')
      config.write('flatten,fft1D_512,336\n')
      config.write('flatten,fft1D_512,352\n')
      config.write('flatten,fft1D_512,367\n')
      config.write('flatten,fft1D_512,392\n')
    else:
      config.write('unrolling,fft1D_512,154,%s\n' %(unroll))
      config.write('unrolling,fft1D_512,201,%s\n' %(unroll))
      config.write('unrolling,fft1D_512,215,%s\n' %(unroll))
      config.write('unrolling,fft1D_512,231,%s\n' %(unroll))
      config.write('unrolling,fft1D_512,246,%s\n' %(unroll))
      config.write('unrolling,fft1D_512,271,%s\n' %(unroll))
      config.write('unrolling,fft1D_512,321,%s\n' %(unroll))
      config.write('unrolling,fft1D_512,336,%s\n' %(unroll))
      config.write('unrolling,fft1D_512,352,%s\n' %(unroll))
      config.write('unrolling,fft1D_512,367,%s\n' %(unroll))
      config.write('unrolling,fft1D_512,392,%s\n' %(unroll))
  elif bench == 'gemm-blocked':
    if unroll >= 8:
      config.write('flatten,bbgemm,45\n')
    else:
      config.write('unrolling,bbgemm,45,%s\n' %(unroll))        # trip_cnt = 8
    config.write('flatten,bbgemm,46\n')
    config.write('flatten,bbgemm,47\n')
    config.write('flatten,bbgemm,48\n')
    config.write('flatten,bbgemm,52\n')
  elif bench == 'gemm-ncubed':
    if unroll >= 64:
      config.write('flatten,gemm,44\n')
    else:
      config.write('unrolling,gemm,44,%s\n' %(unroll))          # trip_cnt = 64
    config.write('flatten,gemm,45\n')
    config.write('flatten,gemm,48\n')
  elif bench == 'kmp-kmp':
    if unroll >= 3:
      config.write('flatten,CPF,40\n')
    else:
      config.write('unrolling,CPF,40,%s\n' %(unroll))               # trip_cnt = 3
    config.write('flatten,CPF,41\n')
    config.write('unrolling,kmp,60,%s\n' %(unroll))                 # trip_cnt = 32411
    config.write('flatten,kmp,61\n')
  elif bench == 'md-knn':
    config.write('unrolling,md_kernel,51,%s\n' %(unroll))       # trip_cnt = 256
    config.write('flatten,md_kernel,58\n')
  elif bench == 'md-grid':
    if unroll <= 4:
      config.write("unrolling,md,46,1\n")
      config.write("unrolling,md,47,1\n")
      config.write("unrolling,md,48,%d\n" %(unroll))
    elif unroll <= 16:
      config.write("unrolling,md,46,1\n")
      config.write("unrolling,md,47,%d\n" %(unroll/4))
      config.write("flatten,md,48\n")
    elif unroll <= 64:
      config.write("unrolling,md,46,%d\n" %(unroll/16))
      config.write("flatten,md,47\n")
      config.write("flatten,md,48\n")
    config.write('flatten,md,50\n')
    config.write('flatten,md,51\n')
    config.write('flatten,md,52\n')
    config.write('flatten,md,56\n')
    config.write('flatten,md,62\n')
  elif bench == 'nw-nw':
#    config.write('unrolling,needwun,45,%s\n' %(unroll))         # trip_cnt = 129
#    config.write('unrolling,needwun,49,%s\n' %(unroll))         # trip_cnt = 129
    config.write('unrolling,needwun,54,%s\n' %(unroll))         # trip_cnt = 128
    config.write('flatten,needwun,55\n')
    config.write('flatten,needwun,99\n')
  elif bench == 'sort-merge':                                   # hard to predict trip counts
    config.write('unrolling,merge,37,%s\n' %(unroll))           # trip_cnt =
    config.write('unrolling,merge,41,%s\n' %(unroll))           # trip_cnt =
    config.write('unrolling,merge,48,%s\n' %(unroll))
    config.write('unrolling,mergesort,69,%s\n' %(unroll))       # trip_cnt =
    config.write('flatten,mergesort,70\n')
  elif bench == 'sort-radix':
    config.write('unrolling,local_scan,41,%s\n' %(unroll))      # trip_cnt = 128
    config.write('flatten,local_scan,42\n')
    config.write('unrolling,sum_scan,53,%s\n' %(unroll))        # trip_cnt = 127
    config.write('unrolling,last_step_scan,62,%s\n' %(unroll))  # trip_cnt = 128
    config.write('flatten,last_step_scan,63')
    config.write('unrolling,init,73,%s\n' %(unroll))            # trip_cnt = 2048
    config.write('unrolling,hist,82,%s\n' %(unroll))            # trip_cnt = 512
    config.write('flatten,hist,83')
    config.write('unrolling,update,96,%s\n' %(unroll))          # trip_cnt = 512
    config.write('flatten,update,97\n')
    if unroll >= 16:
        config.write('flatten,ss_sort,112\n')
    else:
        config.write('unrolling,ss_sort,112,%s\n' %(unroll))    # trip_cnt = 16
  elif bench == 'spmv-crs':
    config.write('unrolling,spmv,46,%s\n' %(unroll))            # trip_cnt = 494
    config.write('flatten,spmv,50\n')
  elif bench == 'spmv-ellpack':
    config.write('unrolling,ellpack,45,%s\n' %(unroll))         # trip_cnt = 494
    config.write('flatten,ellpack,47\n')
  elif bench == 'stencil-stencil2d':
    config.write('unrolling,stencil,40,%s\n' %(unroll))         # trip_cnt = 126
    config.write('flatten,stencil,41\n')
    config.write('flatten,stencil,43\n')
    config.write('flatten,stencil,44\n')
  elif bench == 'stencil-stencil3d':
    if unroll >= 30:
      config.write('flatten,stencil3d,44\n')
    else:
      config.write('unrolling,stencil3d,44,%s\n' %(unroll))     # trip_cnt = 30
    config.write('flatten,stencil3d,45\n')
    config.write('flatten,stencil3d,46\n')
  elif bench == 'viterbi-viterbi':
    config.write('unrolling,viterbi,44,%s\n' %(unroll))         # trip_cnt = 128
    config.write('flatten,viterbi,46\n')
    config.write('flatten,viterbi,47\n')
    if unroll >= 32:
      config.write('flatten,viterbi,58\n')
    else:
      config.write('unrolling,viterbi,58,%s\n' %(unroll))         # trip_cnt = 32
  else:
    print "Need to put an appropriate name of benchmark"

  config.close()

if __name__ == '__main__':
  directory = sys.argv[1]
  kernel = sys.argv[2]
  algorithm = sys.argv[3]
  part = sys.argv[4]
  unroll = sys.argv[5]
  pipe = sys.argv[6]
  cycle_time = sys.argv[7]
  main(directory, kernel, algorithm, part, unroll, pipe, cycle_time)
