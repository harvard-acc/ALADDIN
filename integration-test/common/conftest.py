import pytest
import os
import tempfile
import shutil
import subprocess

RUN_SCRIPT_FNAME = "run.sh"
OUTPUT_DIR = "output_dir"
SWEEP_GENERATOR = "generate_design_sweeps.py"

def is_master(config):
  """Return true we're on a master node."""
  return not hasattr(config, 'workerinput')

def pytest_addoption(parser):
  parser.addoption("--sweep-file", action="store", help="Sweep file")

def pytest_configure(config):
  """Generate sweeps using the sweep generator.

  Only the master node performs the sweep generation.
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
      fmt_key = "%%(%s)s" % OUTPUT_DIR
      for line in f_in:
        if fmt_key in line:
          f_out.write(line % {OUTPUT_DIR: config.tmp_output_dir})
        else:
          f_out.write(line)
    # Generate the sweeps using the generator.
    sweeps_dir = "%s/../../sweeps" % os.environ["ALADDIN_HOME"]
    os.chdir(sweeps_dir)
    sweep_generator = os.path.join(sweeps_dir, SWEEP_GENERATOR)
    process = subprocess.Popen(
        ["python", sweep_generator, config.tmp_sweep_file],
        stdout=True,
        stderr=subprocess.PIPE)
    _, stderr = process.communicate()
    assert process.returncode == 0, (
        "Running the sweeper returned nonzero exit "
        "code Contents of stderr:\n %s" % stderr.decode("utf-8"))
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
  if is_master(config):
    shutil.rmtree(config.tmp_output_dir)
    os.remove(config.tmp_sweep_file)

def pytest_configure_node(node):
  """This hook only runs on the workers to get the shared `run_scripts`."""
  node.workerinput["run_scripts"] = node.config.run_scripts

def pytest_generate_tests(metafunc):
  if is_master(metafunc.config):
    run_scripts = metafunc.config.run_scripts
  else:
    run_scripts = metafunc.config.workerinput["run_scripts"]
  if "run_script" in metafunc.fixturenames:
    metafunc.parametrize("run_script", run_scripts)
