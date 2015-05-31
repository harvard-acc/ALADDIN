#!env python

import os

integration_test_dir = os.getcwd()

standalone_test = integration_test_dir + '/standalone/'

for dir_name in os.listdir(standalone_test):
  print dir_name
  os.chdir(standalone_test + '/' + dir_name)
  os.system('bash run.sh')

with_cpu_test = integration_test_dir + '/with-cpu/'
for dir_name in os.listdir(with_cpu_test):
  print dir_name
  os.chdir(with_cpu_test + '/' + dir_name)
  os.system('bash run.sh')
