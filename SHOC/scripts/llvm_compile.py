#!/usr/bin/env python 
import os
import sys
import operator
import gzip
import math
from collections import defaultdict
from subprocess import Popen
from subprocess import PIPE
from subprocess import STDOUT
import shlex

sources_dict = {
'nnet'    : 'nnet_fwd.c',
'bb_gemm' : 'bb_gemm.c',
'fft'     : 'fft.c',
'md'      : 'md.c',
'pp_scan' : 'pp_scan.c',
'reduction' : 'reduction.c',
'ss_sort' : 'ss_sort.c',
'stencil' : 'stencil.c',
'triad'   : 'triad.c',
}

kernels = {
'nnet-0' :
'nnet_fwd_relu,matrix_vector_product_with_bias,dot_product_kernel_with_bias,RELU,clear_matrix,copy_matrix',
'nnet-1' :
'nnet_fwd_sig,matrix_vector_product_with_bias,dot_product_kernel_with_bias,sigmoid_lookup,clear_matrix,copy_matrix',
'bb_gemm' : 'bb_gemm',
'fft' :
'fft1D_512,step1,step2,step3,step4,step5,step6,step7,step8,step9,step10,step11',
'md' : 'md,md_kernel',
'pp_scan' : 'pp_scan,local_scan,sum_scan,last_step_scan',
'reduction' : 'reduction',
'ss_sort' : 'ss_sort,init,hist,local_scan,sum_scan,last_step_scan,update',
'stencil' : 'stencil',
'triad' : 'triad',
}

top_func = {
'nnet-0' : 'nnet_fwd_relu',
'nnet-1' : 'nnet_fwd_sig',
'bb_gemm' : 'bb_gemm',
'fft' : 'fft1D_512',
'md' : 'md',
'pp_scan' : 'pp_scan',
'reduction' : 'reduction',
'ss_sort' : 'ss_sort',
'stencil' : 'stencil',
'triad' : 'triad',
}


def main (directory, source):
#def main (directory, kernel, algorithm, source):
  if source == 'aes': 
    assert 'MACH_HOME' in os.environ, 'Please set the MACH_HOME environment variable'
    MACH_HOME=os.environ['MACH_HOME']
    test = MACH_HOME+'/common/harness.c'
    test_obj = source + '_test.llvm'
  
  os.chdir(directory)
  obj = source + '.llvm'
  opt_obj = source + '-opt.llvm'
  executable = source + '-instrumented'
  os.environ['WORKLOAD']=kernels[source]
  os.environ['TOPFUNC'] = top_func[source]

  source_file = source + '.c'
  print directory
  os.system('clang -g -O1 -S -fno-slp-vectorize -fno-vectorize -fno-unroll-loops -fno-inline -emit-llvm -o ' + obj + ' '  + source_file)
  if source == 'aes':
    os.system('clang -O2 -S -fno-slp-vectorize -fno-vectorize -fno-unroll-loops -fno-inline -emit-llvm -o ' + test_obj + ' '  + test)
  #os.system('opt -S -load=/group/vlsiarch/shao/Projects/llvm-trace/indvar-list/indvar_list.so -indvar_list ' + obj + ' -o ' + opt_obj)
  os.system('opt -S -load=/group/vlsiarch/shao/Projects/llvm-trace/full-trace/full_trace.so -fulltrace ' + obj + ' -o ' + opt_obj)
  if source == 'aes':
    os.system('llvm-link -o full.llvm ' + opt_obj + ' '+ test_obj+ ' /group/vlsiarch/shao/Projects/llvm-trace/profileFunc/trace_logger.llvm')
  else:
    os.system('llvm-link -o full.llvm ' + opt_obj + ' /group/vlsiarch/shao/Projects/llvm-trace/profileFunc/trace_logger.llvm')

  os.system('llc -O0 -filetype=obj -o full.o full.llvm')
  os.system('llc -O0 -filetype=asm -o full.s full.llvm')
  os.system('gcc -O0 -fno-inline -o ' + executable + ' full.s -lm')
  os.system('./' + executable + ' input.data check.data')

if __name__ == '__main__':
  directory = sys.argv[1]
  source = sys.argv[2]
  print directory, source
  main(directory, source)
