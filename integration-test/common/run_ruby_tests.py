# All integration tests using caches can also be run with Ruby.

import unittest
import os

import gem5_aladdin_test as gat
import run_cpu_tests as tests

class AesTest(tests.AesTest):
  def setSimParams(self):
    super(AesTest, self).setSimParams()
    self.addGem5Parameter({"ruby": True})

  def setExpectedResults(self):
    self.addExpectedStatResult("system.test_aes_datapath.sim_cycles", 8229)

  def runTest(self):
    self.runAndValidate()

class DmaCacheHybridTest(tests.DmaCacheHybridTest):
  def setSimParams(self):
    super(DmaCacheHybridTest, self).setSimParams()
    self.addGem5Parameter({"ruby": True})

  def setExpectedResults(self):
    self.addExpectedStatResult("system.test_hybrid_datapath.sim_cycles", 5326)

  def runTest(self):
    self.runAndValidate()

class LoadStoreTest(tests.LoadStoreTest):
  """ Tests load/store functionality through a cache. """
  def setSimParams(self):
    super(LoadStoreTest, self).setSimParams()
    self.addGem5Parameter({"ruby": True})

  def setExpectedResults(self):
    self.addExpectedStatResult("system.test_load_store_datapath.sim_cycles", 4095)
    self.addExpectedStatResult("system.ruby.l1_cntrl_acc0.L1Dcache.demand_hits", 1919)
    self.addExpectedStatResult("system.ruby.l1_cntrl_acc0.L1Dcache.demand_misses", 258)

  def runTest(self):
    self.runAndValidate()

class MmapTest(tests.MmapTest):
  def setSimParams(self):
    super(MmapTest, self).setSimParams()
    self.addGem5Parameter({"ruby": True})

  def setExpectedResults(self):
    self.addExpectedStatResult("sim_ticks", 766725500)

  def runTest(self):
    self.runAndValidate()

if __name__ == "__main__":
  unittest.main()
