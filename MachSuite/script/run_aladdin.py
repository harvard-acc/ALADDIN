#!/usr/bin/env python
import os
import sys
import os.path
import socket

import llvm_compile
import config

main_kernel_c = {
  'aes-aes' : 'aes',
  'backprop-backprop' : 'net',
  'bfs-bulk' : 'bulk',
  'bfs-queue' : 'queue',
  'kmp-kmp' : 'kmp',
  'fft-strided' : 'fft',
  'fft-transpose' : 'fft',
  'gemm-blocked': 'bbgemm',
  'gemm-ncubed' : 'gemm',
  'md-knn': 'md',
  'md-grid': 'md',
  'nw-nw' : 'needwun',
  'sort-merge' : 'merge',
  'sort-radix' : 'radix',
  'spmv-crs' : 'crs',
  'spmv-ellpack' : 'ellpack',
  'stencil-stencil2d' : 'stencil',
  'stencil-stencil3d' : 'stencil3d',
  'viterbi-viterbi' : 'viterbi'
}

def main(benches, part, unroll, pipe, clock_period):
  print 'Running on %s' % socket.gethostname()

  part = int(part)
  unroll = int(unroll)

  if not 'ALADDIN_HOME' in os.environ:
    raise Exception('Set ALADDIN_HOME directory as an environment variable')

  if not 'TRACER_HOME' in os.environ:
    raise Exception('Set TRACER_HOME directory as an environment variable')

  if not 'MACH_HOME' in os.environ:
    raise Exception('Set MACH_HOME directory as an environment variable')

  ALADDIN_HOME = os.getenv('ALADDIN_HOME')
  MACH_HOME = os.getenv('MACH_HOME')

  if benches == 'all':
    benches = main_kernel_c.iterkeys()
  else:
    benches = benches.split(",")

  for bench in benches:
    print ""
    print '#' * 20
    print "     %s" % bench
    print '#' * 20

    kernel = bench.split('-')[0]
    algorithm = bench.split('-')[1]
    BENCH_HOME = MACH_HOME + '/' + kernel + '/' + algorithm

    print BENCH_HOME
    os.chdir(BENCH_HOME)
    d = 'p%s_u%s_P%s_%sns' % (part, unroll, pipe, clock_period)

    #Run LLVM-Tracer to generate the dynamic trace
    print bench
    source = main_kernel_c[bench]
    llvm_compile.main(BENCH_HOME, bench, source)

    #Generate accelerator design config file
    config.main(BENCH_HOME, kernel, algorithm, part, unroll, pipe, clock_period)

    print 'Start Aladdin'
    trace_file = BENCH_HOME+ '/' + 'dynamic_trace.gz'
    config_file = 'config_' + d

    newdir = os.path.join(BENCH_HOME, 'sim', d)
    print 'Changing directory to %s' % newdir

    os.chdir(newdir)
    os.system('%s/common/aladdin %s %s %s ' % (os.getenv('ALADDIN_HOME'), bench, trace_file, config_file))

if __name__ == '__main__':
  kernel = sys.argv[1]
  part = sys.argv[2]
  unroll = sys.argv[3]
  pipe = sys.argv[4]
  clock_period = sys.argv[5]
  main(kernel, part, unroll, pipe, clock_period)
