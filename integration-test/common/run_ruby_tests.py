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
    self.addExpectedStatResult("system.test_hybrid_datapath.sim_cycles", 7672)

  def runTest(self):
    self.runAndValidate()

class LoadStoreTest(tests.LoadStoreTest):
  """ Tests load/store functionality through a cache. """
  def setSimParams(self):
    super(LoadStoreTest, self).setSimParams()
    self.addGem5Parameter({"ruby": True})

  def setExpectedResults(self):
    self.addExpectedStatResult(
        "system.test_load_store_datapath.sim_cycles", 4095)
    self.addExpectedStatResult(
        "system.ruby.l1_cntrl_acc0.L1Dcache.demand_hits", 1919)
    self.addExpectedStatResult(
        "system.ruby.l1_cntrl_acc0.L1Dcache.demand_misses", 258)

  def runTest(self):
    self.runAndValidate()

class DmaLoadStoreTest(tests.DmaLoadStoreTest):
  def setSimParams(self):
    super(DmaLoadStoreTest, self).setSimParams()
    self.addGem5Parameter({"ruby": True})

  def setExpectedResults(self):
    self.addExpectedStatResult(
        "system.test_dma_load_store_datapath.sim_cycles", 22898)

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

class DoubleBufferingTest(tests.DoubleBufferingTest):
  def setSimParams(self):
    super(DoubleBufferingTest, self).setSimParams()
    self.addGem5Parameter({"ruby": True})

  def setExpectedResults(self):
    self.addExpectedStatResult(
        "system.test_double_buffering_datapath.sim_cycles", 876)

  def runTest(self):
    self.runAndValidate()

class ArrayFunctionRenamedArgTest(tests.ArrayFunctionRenamedArgTest):
  def setSimParams(self):
    super(ArrayFunctionRenamedArgTest, self).setSimParams()
    self.addGem5Parameter({"ruby": True})

  def setExpectedResults(self):
    self.addExpectedStatResult(
        "system.test_array_func_arg_datapath.sim_cycles", 663)

  def runTest(self):
    self.runAndValidate()

class MultipleInvocationsTest(tests.MultipleInvocationsTest):
  def setSimParams(self):
    super(MultipleInvocationsTest, self).setSimParams()
    self.addGem5Parameter({"ruby": True})

  def setExpectedResults(self):
    self.addExpectedStatResult(
        "system.test_multiple_invocations_datapath.sim_cycles", 1212)

  def runTest(self):
    self.runAndValidate()

class ArrayMultidimIndexingTest(tests.ArrayMultidimIndexingTest):
  def setSimParams(self):
    super(ArrayMultidimIndexingTest, self).setSimParams()
    self.addGem5Parameter({"ruby": True})

  def setExpectedResults(self):
    # No need to check for simulated cycles. This is a functionality test only.
    return

  def runTest(self):
    self.runAndValidate()

@unittest.skip("ACP and SIMD tests are skipped for now.")
class AcpTest(tests.AcpTest):
  def setSimParams(self):
    super(AcpTest, self).setSimParams()
    self.addGem5Parameter({"ruby": True})

  def setExpectedResults(self):
    self.addExpectedStatResult("system.test_acp_datapath.sim_cycles", 100)

  def runTest(self):
    self.runAndValidate()

@unittest.skip("ACP and SIMD tests are skipped for now.")
class SimdTest(tests.SimdTest):
  def setSimParams(self):
    super(SimdTest, self).setSimParams()
    self.addGem5Parameter({"ruby": True})

  def setExpectedResults(self):
    self.addExpectedStatResult("system.test_simd_datapath.sim_cycles", 100)

  def runTest(self):
    self.runAndValidate()

class StreamingDmaTest(tests.StreamingDmaTest):
  def setSimParams(self):
    super(StreamingDmaTest, self).setSimParams()
    self.addGem5Parameter({"ruby": True})

  def setExpectedResults(self):
    self.addExpectedStatResult(
        "system.test_streaming_dma_datapath.sim_cycles", 514)

  def runTest(self):
    self.runAndValidate()

if __name__ == "__main__":
  unittest.main()
