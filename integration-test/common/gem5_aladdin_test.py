# Integration test framework for gem5-aladdin.
#
# All integration tests should inherit from this class.
# Framework design inspired by that of XIOSim.

from configparser import ConfigParser
import unittest
import os
import shutil
import subprocess
import tempfile

# Although gem5 is a deterministic simulator, there can be differences in the
# cycle count between runs in different directories. One difference (there may
# be others) is parsing filepaths (which tend to be quite short in the testing
# environment).
SIM_TICKS_ERROR = 0.01

# Default arguments to be passed to the gem5 binary after the setup script.
DEFAULT_POST_SCRIPT_ARGS = {
  "num-cpus": 1,
  "mem-size": "4GB",
  "mem-type": "DDR3_1600_8x8",
  "sys-clock": "1GHz",
  "cpu-type": "DerivO3CPU",
  "caches": True,
  "enable_prefetchers": True,
  "xbar_width": 16,
}

GEM5_CFG_FILE = "gem5.cfg"

class Gem5AladdinTest(unittest.TestCase):
  """ Base end-to-end test class.

  Test cases should inherit from this class, set all the appropriate class
  members and simulation parameters in a setSimParams() function, and
  implement the runTest() function (which usually just needs to call
  runAndValidate()).
  """
  def setUp(self):
    # Aladdin installation directory.
    self.aladdin_home = os.environ["ALADDIN_HOME"]

    # Name of the binary being simulated.
    self.sim_bin = ""

    # Temporary folder containing simulation output.
    self.run_dir = tempfile.mkdtemp()
    os.makedirs(os.path.join(self.run_dir, "outputs"))

    # Delete temporary run folder after test completion?
    self.clean_run_dir = "PRESERVE_RUN_DIR" not in os.environ

    # Directory containing all the test configuration files.
    self.test_dir = ""

    # Whether this test needs only a CPU or if it needs an accelerator too.
    self.cpu_only = False

    # Relative path to the Aladdin test configuration file, from test_dir.
    self.accel_cfg_file = ""

    # Arguments specific to the simulated binary. These will be specified as a
    # space-separated list on the run command.
    self.test_specific_args = []

    # Additional files needed by the test. List of names.
    self.required_files = []

    # All arguments that are passed to the simulation script (rather than to
    # gem5 itself).
    self.sim_script_args = {}

    # Debug flags for the trace output. These will be passed as a
    # comma-separated list.
    self.debug_flags = []

    # Dict of expected test results, where key is a stat name and the value is
    # the expected stat value.
    # TODO: Make key a regex.
    self.expected_results = {}

    # If we're on Jenkins, we have an environment variable directly to the
    # workspace, so we'll just use that. Otherwise, we assume that aladdin is
    # stored under src/aladdin, so we use the ALADDIN_HOME environment
    # variable.
    if "WORKSPACE" in os.environ:
      self.workspace = self.aladdin_home
    else:
      self.workspace = "%s/../.." % self.aladdin_home

    # Invoke the child class simulation parameters function.
    self.setSimParams()

    # Define what the test expects the results to be.
    self.setExpectedResults()

    # Prepare the temporary run directory.
    self.prepareRunDir()

  def tearDown(self):
    """ Delete temporary files and outputs. """
    if self.clean_run_dir:
      shutil.rmtree(self.run_dir)

  def getAladdinConfigFile(self, target_dir):
    """ Return the name of the file specifying Aladdin parameters.

    This can only be reliably done by reading the gem5.cfg file.
    """
    gem5cfg = ConfigParser()
    gem5cfg.read(os.path.join(target_dir, self.accel_cfg_file))
    test = gem5cfg.sections()[0]
    aladdin_cfg = gem5cfg.get(test, "config_file_name")
    return os.path.basename(aladdin_cfg)

  def prepareRunDir(self):
    """ Prepares the temp run dir by putting all config files into place. """
    # Symlink the binary.
    if self.sim_bin:
      os.symlink(os.path.join(self.test_dir, self.sim_bin),
                 os.path.join(self.run_dir, self.sim_bin))
    # Copies CACTI config files.
    common_dir = os.path.join(self.aladdin_home, "integration-test", "common")
    os.symlink(os.path.join(common_dir, "test_cacti_cache.cfg"),
               os.path.join(self.run_dir, "test_cacti_cache.cfg"))
    os.symlink(os.path.join(common_dir, "test_cacti_tlb.cfg"),
               os.path.join(self.run_dir, "test_cacti_tlb.cfg"))

    # Copies any additional required files.
    for f in self.required_files:
      shutil.copy(f, self.run_dir)

    if not self.cpu_only:
      # Symlink the dynamic trace (which could be large).
      for root,dir,files in os.walk(self.test_dir):
        for f in files:
          if f.endswith(".gz"):
            os.symlink(os.path.join(self.test_dir, f),
                       os.path.join(self.run_dir, f))
      # Generates a new gem5.cfg file for the temp run dir.
      self.createGem5Config()
      # Copies the Aladdin array/loop config file
      shutil.copy(os.path.join(
          self.test_dir, self.getAladdinConfigFile(self.run_dir)), self.run_dir)

    run_cmd = self.buildRunCommand()
    with open(os.path.join(self.run_dir, "run.sh"), "w") as f:
      f.write("#!/bin/bash\n")
      f.write(run_cmd)

  def createGem5Config(self):
    """ Creates a new gem5.cfg file for the temp directory.

    The gem5.cfg by default is designed to allow the user to run the test
    manually in that same directory. We have to update paths for the test
    directory.
    """
    gem5cfg = ConfigParser()
    gem5cfg.read(os.path.join(self.test_dir, self.accel_cfg_file))
    # TODO: We only support a single accelerator test at the moment. Expand
    # this when we encounter the need for multi-accelerator tests.
    test = gem5cfg.sections()[0]
    gem5cfg.set(test, "input_dir", self.run_dir)
    gem5cfg.set(test, "cacti_cache_config",
                os.path.join("%(input_dir)s", "test_cacti_cache.cfg"))
    gem5cfg.set(test, "cacti_tlb_config",
                os.path.join("%(input_dir)s", "test_cacti_tlb.cfg"))
    with open(os.path.join(self.run_dir, GEM5_CFG_FILE), "w") as tmp_file:
      gem5cfg.write(tmp_file)
    self.sim_script_args["accel_cfg_file"] = os.path.join(
        self.run_dir, GEM5_CFG_FILE)

  def addRequiredFile(self, name):
    """ Adds a required file to the list.

    This file will be copied to the temp run directory. If the filename begins
    with a slash, it is interpreted as an absolute path; otherwise, it is a
    relative path from test_dir.
    """
    if not name:
      return
    if not name[0] == os.pathsep:
      name = os.path.join(self.test_dir, name)
    self.required_files.append(name)

  def setSimBin(self, binary):
    """ Set the name of the simulated binary. """
    self.sim_bin = binary

  def setTestDir(self, test_dir):
    """ Set the directory containing the test files. """
    self.test_dir = test_dir

  def setGem5CfgFile(self, cfg_file):
    """ Set the relative location of the gem5 configuration file. """
    self.accel_cfg_file = cfg_file

  def setTestSpecificArgs(self, args):
    """ Specify simulated binary specific arguments."""
    self.test_specific_args.extend(args)

  def setCpuOnly(self):
    """ This test does not need any accelerators. """
    self.cpu_only = True

  def addDebugFlags(self, flags):
    """ Add debug flags to the trace output. """
    self.debug_flags.extend(flags)

  def addGem5Parameter(self, params):
    """ Add additional gem5 run parameters, overwriting existing ones.

    Args:
      params: Dict of parameters to values. For switch flags, simply pass True
         as the value.
    """
    for param, value in params.items():
      self.sim_script_args[param] = value

  def addExpectedStatResult(self, stat_name, stat_value):
    """ Add an expected statistic result. """
    self.expected_results[stat_name] = stat_value

  def buildRunCommand(self):
    """ Build the shell command to run gem5. """
    binary = "%s/build/X86/gem5.opt" % self.workspace

    if self.debug_flags:
      debug_flags = "--debug-flags=" + ",".join(self.debug_flags)
    else:
      debug_flags = ""

    out_dir = "--outdir=%s" % os.path.join(self.run_dir, "outputs")

    sim_script = "%s/configs/aladdin/aladdin_se.py" % self.workspace

    combined_sim_script_args = []
    for param, value in self.sim_script_args.items():
      if isinstance(value, bool):
        combined_sim_script_args.append("--%s" % param)
      else:
        combined_sim_script_args.append("--%s=%s" % (param, value))

    for param, value in DEFAULT_POST_SCRIPT_ARGS.items():
      if param not in self.sim_script_args:
        if type(value) == bool:
          combined_sim_script_args.append("--%s" % param)
        else:
          combined_sim_script_args.append("--%s=%s" % (param, value))

    sim_script_args_str = " ".join(combined_sim_script_args)

    if ("num-cpus" in self.sim_script_args and
        self.sim_script_args["num-cpus"] == 0):
      sim_bin = ""
      sim_opt = ""
    else:
      sim_bin = "-c %s" % self.sim_bin
      if (self.test_specific_args):
        sim_opt = "-o \"%s\"" % " ".join(self.test_specific_args)
      else:
        sim_opt = ""

    cmd = ("%(binary)s \\\n"
           "%(debug_flags)s \\\n"
           "%(out_dir)s \\\n"
           "%(sim_script)s \\\n"
           "%(sim_script_args)s \\\n"
           "%(sim_bin)s %(sim_opt)s \\\n") % {
        "binary": binary,
        "debug_flags": debug_flags,
        "out_dir": out_dir,
        "sim_script": sim_script,
        "sim_script_args": sim_script_args_str,
        "sim_bin": sim_bin,
        "sim_opt": sim_opt}

    return cmd

  def parseGem5Stats(self, expected_stats=[]):
    """ Parses the gem5 stats.txt file and returns a dict.

      As an optimization, we can specify the relevant stats so that the
      returned dict is limited in size (as stats.txt can be quite large).
    """
    stats_path = os.path.join(self.run_dir, "outputs", "stats.txt")
    stats_f = open(stats_path, "r")

    # Locate start of stats.
    start_flag = "Begin Simulation Statistics"
    start_found = False
    for line in stats_f:
      if start_flag in line:
        start_found = True
        break
    self.assertTrue(start_found, msg="stats.txt file contained no statistics!")

    parsed_stats = {}
    end_flag = "End Simulation Statistics"
    for line in stats_f:
      if end_flag in line:
        break
      stat = line.split()
      if not stat:
        continue
      stat_name = stat[0]
      # Don't check histogram stats.
      if stat[1] != "|":
        stat_value = float(stat[1])
      if expected_stats and not stat_name in expected_stats:
        continue
      parsed_stats[stat_name] = float(stat_value)

    stats_f.close()

    return parsed_stats

  def parseGem5Trace(self):
    """ Parse the gem5 output trace. To be completed. """
    return

  def runAndValidate(self):
    """ Run the test and validate the results. """
    os.chdir(self.run_dir)
    process = subprocess.Popen("sh run.sh", shell=True,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    stdout, _ = process.communicate()
    with open(os.path.join(self.run_dir, "stdout"), "wb") as f:
      f.write(stdout)
    self.assertEqual(process.returncode, 0, msg="gem5 returned nonzero exit code!")

    expected_stats = [s for s in self.expected_results]
    statistics = self.parseGem5Stats(expected_stats=expected_stats)

    for stat, value in self.expected_results.items():
      delta = SIM_TICKS_ERROR * value;
      self.assertAlmostEqual(
          statistics[stat], value, delta=delta,
          msg="Got %d, expected %d: Difference is more than %d%% of "
          "the expected value" % (statistics[stat], value, SIM_TICKS_ERROR * 100))
