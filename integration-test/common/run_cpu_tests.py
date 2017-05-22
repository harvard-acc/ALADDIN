# All integration tests that involve a CPU and accelerator.
#
# To add another test, create a new class in this file that inherits from
# Gem5AladdinTest.

import unittest
import os

import gem5_aladdin_test as gat

class StandaloneAesTest(gat.Gem5AladdinTest):
  def setSimParams(self):
    aladdin_home = os.environ["ALADDIN_HOME"]
    self.setTestDir(os.path.join(
        aladdin_home, "integration-test", "standalone", "test_aes"))
    self.setTestStandaloneMode()
    self.setGem5CfgFile("gem5.cfg")
    self.addDebugFlags(["HybridDatapath", "Aladdin"])
    self.addGem5Parameter({"cacheline_size": 32})

  def setExpectedResults(self):
    self.addExpectedStatResult("sim_ticks", 7130000)

  def runTest(self):
    self.runAndValidate()

class AesTest(gat.Gem5AladdinTest):
  def setSimParams(self):
    aladdin_home = os.environ["ALADDIN_HOME"]
    self.setTestDir(os.path.join(
        aladdin_home, "integration-test", "with-cpu", "test_aes"))
    self.setSimBin("aes")
    self.addRequiredFile("input.data")
    self.addRequiredFile("check.data")
    self.setTestSpecificArgs(["input.data", "check.data"])
    self.setGem5CfgFile("gem5.cfg")
    self.addDebugFlags(["HybridDatapath", "Aladdin"])
    self.addGem5Parameter({"cacheline_size": 64})

  def setExpectedResults(self):
    self.addExpectedStatResult("sim_ticks", 68999000)

  def runTest(self):
    self.runAndValidate()

class DmaCacheHybridTest(gat.Gem5AladdinTest):
  def setSimParams(self):
    aladdin_home = os.environ["ALADDIN_HOME"]
    self.setTestDir(os.path.join(
        aladdin_home, "integration-test", "with-cpu", "test_hybrid"))
    self.setSimBin("test_hybrid")
    self.setGem5CfgFile("gem5.cfg")
    self.addDebugFlags(["HybridDatapath", "Aladdin"])
    self.addGem5Parameter({"cacheline_size": 64})

  def setExpectedResults(self):
    self.addExpectedStatResult("sim_ticks", 39969000)

  def runTest(self):
    self.runAndValidate()

class LoadStoreTest(gat.Gem5AladdinTest):
  """ Tests load/store functionality through a cache. """
  def setSimParams(self):
    aladdin_home = os.environ["ALADDIN_HOME"]
    self.setTestDir(os.path.join(
        aladdin_home, "integration-test", "with-cpu", "test_load_store"))
    self.setSimBin("test_load_store")
    self.setGem5CfgFile("gem5.cfg")
    self.addDebugFlags(["HybridDatapath", "Aladdin"])
    self.addGem5Parameter({"cacheline_size": 64})

  def setExpectedResults(self):
    # Result depends mostly on cache queue size and bandwidth.
    self.addExpectedStatResult("sim_ticks", 36276000)

  def runTest(self):
    self.runAndValidate()

class DmaLoadStoreTest(gat.Gem5AladdinTest):
  def setSimParams(self):
    aladdin_home = os.environ["ALADDIN_HOME"]
    self.setTestDir(os.path.join(
        aladdin_home, "integration-test", "with-cpu", "test_dma_load_store"))
    self.setSimBin("test_dma_load_store")
    self.setGem5CfgFile("gem5.cfg")
    self.addDebugFlags(["HybridDatapath", "Aladdin"])
    self.addGem5Parameter({"cacheline_size": 32})

  def setExpectedResults(self):
    self.addExpectedStatResult("sim_ticks", 60013000)

  def runTest(self):
    self.runAndValidate()

class MmapTest(gat.Gem5AladdinTest):
  def setSimParams(self):
    aladdin_home = os.environ["ALADDIN_HOME"]
    self.setTestDir(os.path.join(
        aladdin_home, "integration-test", "with-cpu", "test_mmap"))
    self.setSimBin("test_mmap")
    self.setCpuOnly()

  def setExpectedResults(self):
    self.addExpectedStatResult("sim_ticks", 686975500)

  def runTest(self):
    self.runAndValidate()

class DoubleBufferingTest(gat.Gem5AladdinTest):
  def setSimParams(self):
    aladdin_home = os.environ["ALADDIN_HOME"]
    self.setTestDir(os.path.join(
        aladdin_home, "integration-test", "with-cpu", "test_double_buffering"))
    self.setSimBin("test_double_buffering")
    self.setGem5CfgFile("gem5.cfg")
    self.addDebugFlags(["HybridDatapath", "Aladdin"])
    self.addGem5Parameter({"cacheline_size": 32})

  def setExpectedResults(self):
    self.addExpectedStatResult("sim_ticks", 25251000)

  def runTest(self):
    self.runAndValidate()

class ArrayFunctionRenamedArgTest(gat.Gem5AladdinTest):
  def setSimParams(self):
    aladdin_home = os.environ["ALADDIN_HOME"]
    self.setTestDir(os.path.join(
        aladdin_home, "integration-test", "with-cpu", "test_array_func_arg"))
    self.setSimBin("test_array_func_arg")
    self.setGem5CfgFile("gem5.cfg")
    self.addDebugFlags(["HybridDatapath", "Aladdin"])
    self.addGem5Parameter({"cacheline_size": 32})

  def setExpectedResults(self):
    self.addExpectedStatResult("sim_ticks", 25243000)

  def runTest(self):
    self.runAndValidate()

class MultipleInvocationsTest(gat.Gem5AladdinTest):
  def setSimParams(self):
    aladdin_home = os.environ["ALADDIN_HOME"]
    self.setTestDir(os.path.join(
        aladdin_home, "integration-test", "with-cpu", "test_multiple_invocations"))
    self.setSimBin("test_multiple_invocations")
    self.setGem5CfgFile("gem5.cfg")
    self.addDebugFlags(["HybridDatapath", "Aladdin"])
    self.addGem5Parameter({"cacheline_size": 32})

  def setExpectedResults(self):
    self.addExpectedStatResult("sim_ticks", 34168500)

  def runTest(self):
    self.runAndValidate()

class ArrayMultidimIndexingTest(gat.Gem5AladdinTest):
  def setSimParams(self):
    aladdin_home = os.environ["ALADDIN_HOME"]
    self.setTestDir(os.path.join(
        aladdin_home, "integration-test", "with-cpu", "test_array_indexing"))
    self.setSimBin("test_array_indexing")
    self.setGem5CfgFile("gem5.cfg")
    self.addDebugFlags(["HybridDatapath", "Aladdin"])
    self.addGem5Parameter({"cacheline_size": 32})

  def setExpectedResults(self):
    # No need to check for simulated cycles. This is a functionality test only.
    return

  def runTest(self):
    self.runAndValidate()

if __name__ == "__main__":
  unittest.main()
