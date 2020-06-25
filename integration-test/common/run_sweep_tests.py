import os
import subprocess
import six

def test_point(run_script):
  process = subprocess.Popen(["bash", os.path.basename(run_script)],
                             cwd=os.path.dirname(run_script),
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
  stdout, stderr = process.communicate()
  assert process.returncode == 0, (
      "Running a sweep point returned nonzero exit code! Contents of output:\n "
      "%s\n%s" % (six.ensure_text(stdout), six.ensure_text(stderr)))
