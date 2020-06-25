""" Implements pytest hooks to run MachSuite tests on Aladdin.

pytest will look for a conftest.py for directory-specific pytest hook
implementations (tests in the same directory will share the hooks). All the hook
functions have a pytest_ prefix, tests will invoke the hooks defined in
conftest.py. Refer to https://docs.pytest.org/en/stable/reference.html for more
details about the pytest APIs.

Also a bit introduction on pytest-xdist. It's a plugin to pytest that extends
pytest mainly for parallel test run. Simply use the "-n" command line option to
specify the number of processes to run the tests. xdist uses a master node for
control process and worker nodes for running actual tests. Each node will start
a pytest session to collect the tests (they must see the tests consistent
across all nodes), and then the tests will be distributed to all the workers.
"""

import pytest
import os
import tempfile
import shutil
import subprocess
import six

RUN_SCRIPT_FNAME = "run.sh"
OUTPUT_DIR = "output_dir"
SWEEP_GENERATOR = "generate_design_sweeps.py"

def is_master(config):
  """Return true we're on a master node."""
  return not hasattr(config, 'workerinput')

def pytest_addoption(parser):
  """Add an option for specifying the sweep file.

  This hook allows the test to take in command line arguments. This is called
  once at the beginning of a test run. We add a sweep file command line option.
  """
  parser.addoption("--sweep-file", action="store", help="Sweep file")

def pytest_configure(config):
  """Generate sweeps using the sweep generator.

  This hook performs initial configuration. This is called after command line
  options have been parsed. We use this hook to invoke the sweep generator to
  create sweeps. Only the master node performs the sweep generation.
  """
  if is_master(config):
    cwd = os.getcwd()
    config.tmp_output_dir = tempfile.mkdtemp(dir=cwd)
    _, config.tmp_sweep_file = tempfile.mkstemp(suffix=".xe", dir=cwd)
    # Read the sweep file and update the `output_dir` field with a temp dir.
    sweep_file = config.getoption("--sweep-file")
    if not os.path.isabs(sweep_file):
      sweep_file = os.path.join(cwd, sweep_file)
    with open(sweep_file, "r") as f_in, open(config.tmp_sweep_file,
                                             "w") as f_out:
      f_in_content = f_in.read()
      fmt_key = "%%(%s)s" % OUTPUT_DIR
      if fmt_key in f_in_content:
        f_out.write(f_in_content % {OUTPUT_DIR: config.tmp_output_dir})
    # Generate the sweeps using the generator.
    sweeps_dir = "%s/../../sweeps" % os.environ["ALADDIN_HOME"]
    os.chdir(sweeps_dir)
    sweep_generator = os.path.join(sweeps_dir, SWEEP_GENERATOR)
    process = subprocess.Popen(
        ["python", sweep_generator, config.tmp_sweep_file],
        # Print the stdout directly.
        stdout=None,
        stderr=subprocess.PIPE)
    _, stderr = process.communicate()
    assert process.returncode == 0, (
        "Running the sweeper returned nonzero exit "
        "code Contents of stderr:\n %s" % six.ensure_text(stderr))
    os.chdir(cwd)
    # Find all the run scripts.
    run_scripts = []
    for root, dirs, files in os.walk(config.tmp_output_dir):
      for f in files:
        if f == RUN_SCRIPT_FNAME:
          run_scripts.append(os.path.join(root, f))
    # Set the shared `run_scripts`. This can be accessed by the workers later.
    config.run_scripts = run_scripts

def pytest_unconfigure(config):
  """Delete test temporary files.

  This is called before test run is exited. We implement this to delete
  temporary test files.
  """
  if is_master(config):
    shutil.rmtree(config.tmp_output_dir)
    os.remove(config.tmp_sweep_file)

def pytest_configure_node(node):
  """This hook only runs on the workers to get the shared `run_scripts`.

  This is a pytest-xdist hook. It configures worker nodes. It only runs on a
  worker node. We implement this to make the master share the test run scripts
  with the workers.
  """
  node.workerinput["run_scripts"] = node.config.run_scripts

def pytest_generate_tests(metafunc):
  """Generate a test for each run script in the shared `run_scripts`.

  This can be used to generate (multiple) parametrized calls to a test function.
  We use the shared `run_scripts` to dynamically generate tests (one for each
  run script).
  """
  if is_master(metafunc.config):
    run_scripts = metafunc.config.run_scripts
  else:
    run_scripts = metafunc.config.workerinput["run_scripts"]
  if "run_script" in metafunc.fixturenames:
    metafunc.parametrize("run_script", run_scripts)
