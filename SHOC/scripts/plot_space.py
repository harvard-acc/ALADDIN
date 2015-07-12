import numpy as np
import sys
import os
import os.path
import math
import matplotlib

matplotlib.use('Agg')
import matplotlib.pyplot as plt

def main (ALADDIN_HOME, bench):
  cycle = []
  total_power = []
  fu_power = []
  mem_power = []
  total_area = []
  fu_area = []
  mem_area = []


  BENCH_HOME = ALADDIN_HOME + '/SHOC/' + bench
  if not os.path.exists(ALADDIN_HOME + '/SHOC/scripts/data/'):
    os.makedirs(ALADDIN_HOME + '/SHOC/scripts/data/')

  os.chdir(BENCH_HOME + '/sim/')

  dirs = os.listdir(BENCH_HOME + '/sim/')

  for d in dirs:
    if os.path.isfile(d + '/' + bench + '_summary'):
      file = open( d + '/' + bench  + '_summary', 'r')
      for line in file:

        if 'Cycle' in line:
          cycle.append(int(line.split(' ')[2]))
        elif 'Avg Power' in line:
          total_power.append(float(line.split(' ')[2]))
        elif 'Avg FU Power' in line:
          fu_power.append(float(line.split(' ')[3]))
        elif 'Avg MEM Power' in line:
          mem_power.append(float(line.split(' ')[3]))
        elif 'Total Area' in line:
          total_area.append(float(line.split(' ')[2]))
        elif 'FU Area' in line:
          fu_area.append(float(line.split(' ')[2]))
        elif 'MEM Area' in line:
          mem_area.append(float(line.split(' ')[2]))
        else:
          continue
      file.close()

  os.chdir(ALADDIN_HOME + '/SHOC/scripts/data/')
  fig = plt.figure()
  fig.suptitle('SHOC ' + bench + ' Total Power Design Space')
  curr_plot = fig.add_subplot(111)
  curr_plot.scatter(cycle, total_power)
  curr_plot.set_xlabel('Cycles')
  curr_plot.set_ylabel('Total Power (mW)')
  curr_plot.grid(True)
  plt.savefig(bench + '-cycles-total-power.pdf')

  fig = plt.figure()
  fig.suptitle('SHOC ' + bench + ' FU Power Design Space')
  curr_plot = fig.add_subplot(111)
  curr_plot.scatter(cycle, fu_power)
  curr_plot.set_xlabel('Cycles')
  curr_plot.set_ylabel('FU Power (mW)')
  curr_plot.grid(True)
  plt.savefig(bench + '-cycles-fu-power.pdf')

  fig = plt.figure()
  fig.suptitle('SHOC ' + bench + ' MEM Power Design Space')
  curr_plot = fig.add_subplot(111)
  curr_plot.scatter(cycle, mem_power)
  curr_plot.set_xlabel('Cycles')
  curr_plot.set_ylabel('MEM Power (mW)')
  curr_plot.grid(True)
  plt.savefig(bench + '-cycles-mem-power.pdf')


  fig = plt.figure()
  fig.suptitle('SHOC ' + bench + ' Total Area Design Space')
  curr_plot = fig.add_subplot(111)
  curr_plot.scatter(cycle, total_area)
  curr_plot.set_xlabel('Cycles')
  curr_plot.set_ylabel('Total Area (uM^2)')
  curr_plot.grid(True)
  plt.savefig(bench + '-cycles-total-area.pdf')

  fig = plt.figure()
  fig.suptitle('SHOC ' + bench + ' FU Area Design Space')
  curr_plot = fig.add_subplot(111)
  curr_plot.scatter(cycle, fu_area)
  curr_plot.set_xlabel('Cycles')
  curr_plot.set_ylabel('FU Area (uM^2)')
  curr_plot.grid(True)
  plt.savefig(bench + '-cycles-fu-area.pdf')

  fig = plt.figure()
  fig.suptitle('SHOC ' + bench + ' MEM Area Design Space')
  curr_plot = fig.add_subplot(111)
  curr_plot.scatter(cycle, mem_area)
  curr_plot.set_xlabel('Cycles')
  curr_plot.set_ylabel('MEM Area (uM^2)')
  curr_plot.grid(True)
  plt.savefig(bench + '-cycles-mem-area.pdf')
  os.chdir(ALADDIN_HOME + '/SHOC/scripts/')


if __name__ == '__main__':
  bench = sys.argv[1]
  ALADDIN_HOME = str(os.getenv('ALADDIN_HOME'))
  main(ALADDIN_HOME, bench)
