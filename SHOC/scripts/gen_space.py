#!/usr/bin/env python

import numpy as np
import os

import plot_space

unroll = [1, 2, 4, 8, 16, 32, 64]
part  = [1, 2, 4, 8, 16, 32, 64]
pipe = [0,1]

#You can parallel this by running multiply jobs together
#In this case, llvm_coompile.py inside run_aladdin.py only need to run once
#to generate the dynamic instruction trace


benchmarks = ['triad', 'reduction', 'stencil', 'fft', 'md', 'pp_scan', 'bb_gemm']
ALADDIN_HOME = str(os.getenv('ALADDIN_HOME'))

for bench in benchmarks:
  print bench

  for f_unroll in unroll:
    for f_part in part:
      for f_pipe in pipe:
        os.system('python run_aladdin.py %s %i %i %i 6' % (bench, f_part, f_unroll, f_pipe))

  plot_space.main(ALADDIN_HOME, bench)
