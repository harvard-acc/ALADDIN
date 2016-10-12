#!env python
#
# Convenience script to run all integration tests. Does not perform any
# checking to make sure that they have passed - the user has to do this
# manually for now.

import os

integration_test_dir = os.path.join(os.environ["ALADDIN_HOME"], "integration-test")

standalone_test = integration_test_dir + '/standalone/'
for dir_name in os.listdir(standalone_test):
  print "\n\n"
  print "*******************************************"
  print "standalone/%s" % dir_name
  os.chdir(standalone_test + '/' + dir_name)
  os.system('bash run.sh')

with_cpu_test = integration_test_dir + '/with-cpu/'
for dir_name in os.listdir(with_cpu_test):
  print "\n\n"
  print "*******************************************"
  print "with-cpu/%s" % dir_name
  os.chdir(with_cpu_test + '/' + dir_name)
  os.system('bash run.sh')
