from m5.params import *
from m5.objects import CommMonitor
from MemObject import MemObject
from m5.proxy import *

class HybridDatapath(MemObject):
  type = "HybridDatapath"
  cxx_header = "aladdin/gem5/HybridDatapath.h"
  # MemObject is assumed to be a parent.
  cxx_bases = ["ScratchpadDatapath"]

  benchName = Param.String("Aladdin accelerator name.")
  outputPrefix = Param.String("Aladdin output prefix.")
  traceFileName = Param.String("Aladdin Input Trace File")
  configFileName = Param.String("Aladdin Config File")
  cycleTime = Param.Unsigned(1, "Clock Period: 1ns default")
  acceleratorName = Param.String("", "Unique accelerator name")
  acceleratorId = Param.Int(-1, "Accelerator Id")
  system = Param.System(Parent.any, "system object")
  executeStandalone = Param.Bool(True, "Execute Aladdin standalone, without a "
      "CPU/user program.")
  useDb = Param.Bool(False, "Store results in database.")
  experimentName = Param.String("NULL", "Experiment name. String identifier "
      "for a set of related simulations.")

  # Scratchpad/DMA parameters.
  dmaSetupOverhead = Param.Unsigned(30, "Overhead in starting a DMA transaction.")
  maxDmaRequests = Param.Unsigned(16, "Max number of outstanding DMA requests")
  numDmaChannels = Param.Unsigned(16, "Number of virtual DMA channels.")
  dmaChunkSize = Param.Unsigned("64", "DMA transaction chunk size.")
  spadPorts = Param.Unsigned(1, "Scratchpad ports per partition")
  pipelinedDma = Param.Bool(False, "Issue DMA ops as soon as they have "
      "incurred their own wating setup, without waiting for other DMA ops "
      "to finish first.")
  ignoreCacheFlush = Param.Bool(False, "Ignore Cache Flush latency.")
  invalidateOnDmaStore = Param.Bool(True, "Invalidate the region of memory "
      "that will be modified by a dmaStore before issuing the DMA request.")

  # Cache parameters.
  cacheSize = Param.String("16kB", "Private cache size")
  cacheLineSize = Param.Int("64", "Cache line size (in bytes)")
  cacheAssoc = Param.Int(1, "Private cache associativity")
  l2cacheSize = Param.String("128kB", "Shared L2 cache size")
  cacheHitLatency = Param.Int(1, "Hit latency")
  cactiCacheConfig = Param.String("", "CACTI cache config file")
  cactiCacheQueueConfig = Param.String("", "CACTI cache queue config file")
  tlbEntries = Param.Int(0, "number entries in TLB (0 implies infinite)")
  tlbAssoc = Param.Int(4, "Number of sets in the TLB")
  tlbHitLatency = Param.Cycles(0, "number of cycles for a hit")
  tlbMissLatency = Param.Cycles(10, "number of cycles for a miss")
  tlbPageBytes = Param.Int(4096, "Page Size")
  tlbCactiConfig = Param.String("", "TLB CACTI configuration file.")
  numOutStandingWalks = Param.Int(4, "num of outstanding page walks")
  tlbBandwidth = Param.Int(
      1, "Number of translations that can be requested per cycle.")
  cacheQueueSize = Param.Int(32, "Maximum outstanding cache requests.")
  cacheBandwidth = Param.Int(4, "Maximum cache requests per cycle.")

  enableStatsDump = Param.Bool(
      False, "Dump m5 stats after each accelerator invocation.")
  recordMemoryTrace = Param.Bool(
      False, "Record memory traffic going to/from the accelerator.")

  spad_port = MasterPort("HybridDatapath DMA port")
  cache_port = MasterPort("HybridDatapath cache coherent port")

  # HACK: We don't have a scratchpad object. Currently we just connect the
  # scratchpad port inside datapath directly to the memory bus.

  def connectThroughMonitor(self, monitor_name, master_port, slave_port):
    """ Connect the master and slave port through a CommMonitor. """
    trace_file_name = "%s.%s.gz" % (self.benchName, monitor_name)
    monitor = CommMonitor.CommMonitor(trace_enable=True, trace_file=trace_file_name)
    monitor.slave = master_port
    monitor.master = slave_port
    setattr(self, monitor_name, monitor)

  def connectPrivateScratchpad(self, system, bus):
    if self.recordMemoryTrace:
      monitor_name = "spad_monitor"
      self.connectThroughMonitor(monitor_name, self.spad_port, bus.slave)
    else:
      self.spad_port = bus.slave

  def addPrivateL1Dcache(self, system, bus, dwc = None) :
    self.cache_port = self.cache.cpu_side

    if self.recordMemoryTrace:
      monitor_name = "cache_monitor"
      self.connectThroughMonitor(monitor_name, self.cache.mem_side, bus.slave)
    else:
      self.cache.mem_side = bus.slave
