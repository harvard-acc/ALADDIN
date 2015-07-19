# Integration test framework for gem5-aladdin.
#
# All integration tests should inherit from this class.
# Framework design inspired by that of XIOSim.

import ConfigParser
import unittest
import os
import shutil
import subprocess
import tempfile

# Default arguments to be passed to the gem5 binary after the setup script.
DEFAULT_POST_SCRIPT_ARGS = {
  "num-cpus": 1,
  "mem-size": "4GB",
  "mem-type": "ddr3_1600_x64",
  "sys-clock": "1GHz",
  "cpu-type": "timing",
  "caches": True
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
    # TODO: Rename this to gem5_cfg_file after the parameter has been
    # appropriately renamed in aladdin_se.py.
    self.aladdin_cfg_file = ""

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
    gem5cfg = ConfigParser.SafeConfigParser()
    gem5cfg.read(os.path.join(target_dir, self.aladdin_cfg_file))
    test = gem5cfg.sections()[0]
    aladdin_cfg = gem5cfg.get(test, "config_file_name")
    return os.path.basename(aladdin_cfg)

  def prepareRunDir(self):
    """ Prepares the temp run dir by putting all config files into place. """
    # Copies CACTI config files.
    common_dir = os.path.join(self.aladdin_home, "integration-test", "common")
    shutil.copy(os.path.join(common_dir, "test_cacti_cache.cfg"), self.run_dir)
    shutil.copy(os.path.join(common_dir, "test_cacti_tlb.cfg"), self.run_dir)
    shutil.copy(os.path.join(common_dir, "test_cacti_lq.cfg"), self.run_dir)
    shutil.copy(os.path.join(common_dir, "test_cacti_sq.cfg"), self.run_dir)

    # Copies any additional required files.
    for f in self.required_files:
      shutil.copy(f, self.run_dir)

    if not self.cpu_only:
      # Symlink the dynamic trace (which could be large).
      os.symlink(os.path.join(self.test_dir, "dynamic_trace.gz"),
                 os.path.join(self.run_dir, "dynamic_trace.gz"))
      # Generates a new gem5.cfg file for the temp run dir.
      test_name = self.createGem5Config()
      # Copies the Aladdin array/loop config file
      shutil.copy(os.path.join(
          self.test_dir, self.getAladdinConfigFile(self.run_dir)), self.run_dir)

  def createGem5Config(self):
    """ Creates a new gem5.cfg file for the temp directory.

    The gem5.cfg by default is designed to allow the user to run the test
    manually in that same directory. We don't want this for automatic testing,
    since it's cleaner to put everything into its own temp directory, so we're
    going to rewrite the gem5.cfg file for the temp directory.

    Returns: the name of the test.
    """
    gem5cfg = ConfigParser.SafeConfigParser()
    gem5cfg.read(os.path.join(self.test_dir, self.aladdin_cfg_file))
    # TODO: We only support a single accelerator test at the moment. Expand
    # this when we encounter the need for multi-accelerator tests.
    test = gem5cfg.sections()[0]
    gem5cfg.set(test, "input_dir", self.run_dir)
    gem5cfg.set(test, "cacti_cache_config",
                os.path.join("%(input_dir)s", "test_cacti_cache.cfg"))
    gem5cfg.set(test, "cacti_tlb_config",
                os.path.join("%(input_dir)s", "test_cacti_tlb.cfg"))
    gem5cfg.set(test, "cacti_lq_config",
                os.path.join("%(input_dir)s", "test_cacti_lq.cfg"))
    gem5cfg.set(test, "cacti_sq_config",
                os.path.join("%(input_dir)s", "test_cacti_sq.cfg"))
    with open(os.path.join(self.run_dir, GEM5_CFG_FILE), "w") as tmp_file:
      gem5cfg.write(tmp_file)
    self.sim_script_args["aladdin_cfg_file"] = os.path.join(
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
    self.aladdin_cfg_file = cfg_file

  def setTestSpecificArgs(self, args):
    """ Specify simulated binary specific arguments."""
    self.test_specific_args.extend(args)

  def setTestStandaloneMode(self):
    """ Make this test standalone (no CPU). """
    self.sim_script_args["num-cpus"] = 0

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
    for param, value in params.iteritems():
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

    combined_sim_script_args = ["--%s=%s" % (param, value)
            for (param, value) in self.sim_script_args.iteritems()]
    for param, value in DEFAULT_POST_SCRIPT_ARGS.iteritems():
      if param not in self.sim_script_args:
        if type(value) == bool:
          combined_sim_script_args.append("--%s" % param)
        else:
          combined_sim_script_args.append("--%s=%s" % (param, value))

    sim_script_args_str = " ".join(combined_sim_script_args)

    sim_bin = "-c %s" % os.path.join(self.test_dir, self.sim_bin)
    if (self.test_specific_args):
      sim_opt = "-o \"%s\"" % " ".join(self.test_specific_args)
    else:
      sim_opt = ""

    cmd = ("%(binary)s %(debug_flags)s %(out_dir)s %(sim_script)s "
           "%(sim_script_args)s %(sim_bin)s %(sim_opt)s > "
           "%(run_dir)s/stdout.gz") % {
        "binary": binary,
        "debug_flags": debug_flags,
        "out_dir": out_dir,
        "sim_script": sim_script,
        "sim_script_args": sim_script_args_str,
        "sim_bin": sim_bin,
        "sim_opt": sim_opt,
        "run_dir": self.run_dir}

    return cmd

  def parseGem5Stats(self, stats=[]):
    """ Parses the gem5 stats.txt file and returns a dict.

      As an optimization, we can specify the relevant stats so that the
      returned dict is limited in size (as stats.txt can be quite large).
    """
    stats_path = os.path.join(self.run_dir, "outputs", "stats.txt")
    print stats_path
    stats_f = open(stats_path, "r")

    # Locate start of stats.
    start_flag = "Begin Simulation Statistics"
    start_found = False
    for line in stats_f:
      if start_flag in line:
        start_found = True
        break
    self.assertTrue(start_found, msg="stats.txt file contained no statistics!")

    stats = {}
    end_flag = "End Simulation Statistics"
    for line in stats_f:
      if end_flag in line:
        break
      stat = line.split()
      if not stat:
        continue
      stats[stat[0]] = float(stat[1])

    stats_f.close()

    return stats

  def parseGem5Trace(self):
    """ Parse the gem5 output trace. To be completed. """
    return

  def runAndValidate(self):
    """ Run the test and validate the results. """
    run_cmd = self.buildRunCommand()
    print run_cmd
    pwd = os.getcwd()

    os.chdir(self.run_dir)
    returncode = subprocess.call(run_cmd, shell=True)
    self.assertEqual(returncode, 0, msg="gem5 returned nonzero exit code!")
    statistics = self.parseGem5Stats()

    for stat, value in self.expected_results.iteritems():
      self.assertEqual(statistics[stat], value)

    os.chdir(pwd)
