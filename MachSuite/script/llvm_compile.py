#!/usr/bin/env python
import os
import sys

import string
import random

def id_generator(size=6, chars=string.ascii_uppercase + string.digits):
   return ''.join(random.choice(chars) for _ in range(size))

kernels = {
  'aes-aes' : 'gf_alog,gf_log,gf_mulinv,rj_sbox,rj_xtime,aes_subBytes,aes_addRoundKey,aes_addRoundKey_cpy,aes_shiftRows,aes_mixColumns,aes_expandEncKey,aes256_encrypt_ecb',
  'backprop-backprop':'sigmoid,update_layer,update,propagate_error_out,propagate_error_layer,update_weights,propagate_errors,comp_error,backprop',
  'bfs-bulk' : 'bfs',
  'bfs-queue' : 'bfs',
  'kmp-kmp' : 'CPF,kmp',
  'fft-strided' : 'fft',
  'fft-transpose':'twiddles8,loadx8,loady8,fft1D_512',
  'gemm-blocked': 'bbgemm',
  'gemm-ncubed' : 'gemm',
  'md-grid':'md',
  'md-knn':'md_kernel',
  'nw-nw' : 'needwun',
  'sort-merge' : 'merge,mergesort',
  'sort-radix' : 'local_scan,sum_scan,last_step_scan,init,hist,update,ss_sort',
  'spmv-crs' : 'spmv',
  'spmv-ellpack' : 'ellpack',
  'stencil-stencil2d' : 'stencil',
  'stencil-stencil3d' : 'stencil3d',
  'viterbi-viterbi' : 'viterbi',
}

def main (directory, bench, source):

  if not 'TRACER_HOME' in os.environ:
    raise Exception('Set TRACER_HOME directory as an environment variable')
  if not 'MACH_HOME' in os.environ:
    raise Exception('Set MACH_HOME directory as an environment variable')

  #id = id_generator()

  os.chdir(directory)
  obj = source + '.llvm'
  opt_obj = source + '-opt.llvm'
  executable = source + '-instrumented'
  os.environ['WORKLOAD']=kernels[bench]

  test = os.getenv('MACH_HOME')+'/common/harness.c'
  test_obj = source + '_test.llvm'

  source_file = source + '.c'

  #for key in os.environ.keys():
  #  print "%30s %s" % (key,os.environ[key])

  print directory
  print '======================================================================'
  command = 'clang -g -O1 -S -I' + os.environ['ALADDIN_HOME'] + \
            ' -fno-slp-vectorize -fno-vectorize -fno-unroll-loops ' + \
            ' -fno-inline -fno-builtin -emit-llvm -o ' + obj + ' ' + source_file
  print command
  os.system(command)
  command = 'clang -g -O1 -S -I' + os.environ['ALADDIN_HOME'] + \
            ' -fno-slp-vectorize -fno-vectorize -fno-unroll-loops ' + \
            ' -fno-inline -fno-builtin -emit-llvm -o ' + test_obj + ' '  + test
  print command
  os.system(command)
  command = 'opt -S -load=' + os.getenv('TRACER_HOME') + \
            '/full-trace/full_trace.so -fulltrace ' + obj + ' -o ' + opt_obj
  print command
  os.system(command)
  command = 'llvm-link -o full.llvm ' + opt_obj + ' ' + test_obj + ' ' + \
            os.getenv('TRACER_HOME') + '/profile-func/trace_logger.llvm'
  print command
  os.system(command)
  command = 'llc -O0 -disable-fp-elim -filetype=asm -o full.s full.llvm'
  print command
  os.system(command)
  command = 'gcc -O0 -fno-inline -o ' + executable + ' full.s -lm -lz'
  print command
  os.system(command)
  command = './' + executable + ' input.data check.data'
  print command
  os.system(command)
  print '======================================================================'

if __name__ == '__main__':
  directory = sys.argv[1]
  bench = sys.argv[2]
  source = sys.argv[3]
  print directory, bench, source
  main(directory, bench, source)
