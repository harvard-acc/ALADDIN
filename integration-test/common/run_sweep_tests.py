import os
import subprocess
import six

def test_point(run_script):
  process = subprocess.Popen(["bash", os.path.basename(run_script)],
                             cwd=os.path.dirname(run_script),
                             stdout=None,
                             stderr=subprocess.PIPE)
  _, stderr = process.communicate()
  assert process.returncode == 0, (
      "Running a sweep point returned nonzero exit code! Contents of stderr:\n "
      "%s" % six.ensure_text(stderr))
