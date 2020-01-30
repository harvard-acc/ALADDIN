from m5.params import *
from m5.objects import CommMonitor, Cache, MemTraceProbe
from MemObject import MemObject
from m5.proxy import *
from Cache import *
from XBar import *

class AcpCache(Cache):
  """ A small L1 cache that handles coherency logic on ACP's behalf.

  Since this is just to get coherency functionality that we would ordinarily
  implement on our own in zero time, this needs to be very small (just one or
  two cache lines), but latency-wise, it needs to emulate the characteristics
  of an L2 cache lookup. However, we do need to ensure we have enough MSHRs and
  write buffers so that we don't suffer MORE than an L2 cache lookup latency when
  we have to writeback cache lines to the L2.
  """
  size = "128B"
  assoc = 1
  tag_latency = 1
  data_latency = 1
  response_latency = 1
  mshrs = 4
  tgts_per_mshr = 4
  write_buffers = 4
  prefetcher = NULL

class HybridDatapath(ClockedObject):
  type = "HybridDatapath"
  cxx_header = "aladdin/gem5/HybridDatapath.h"
  # MemObject is assumed to be a parent.
  cxx_extra_bases = ["ScratchpadDatapath"]

  benchName = Param.String("Aladdin accelerator name.")
  outputPrefix = Param.String("Aladdin output prefix.")
  traceFileName = Param.String("Aladdin Input Trace File")
  configFileName = Param.String("Aladdin Config File")
  cycleTime = Param.Unsigned(1, "Clock Period: 1ns default")
  acceleratorName = Param.String("", "Unique accelerator name")
  acceleratorId = Param.Int(-1, "Accelerator Id")
  system = Param.System(Parent.any, "system object")
  useDb = Param.Bool(False, "Store results in database.")
  experimentName = Param.String("NULL", "Experiment name. String identifier "
      "for a set of related simulations.")

  # Scratchpad/DMA parameters.
  cacheLineFlushLatency = Param.Unsigned(113, "Latency of flushing a cacheline (ns).")
  cacheLineInvalidateLatency = Param.Unsigned(113, "Latency invalidating a cacheline (ns).")
  dmaSetupOverhead = Param.Unsigned(234, "Overhead in starting a DMA transaction (ns).")
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

  # ACP cache latency parameters
  acpCacheSize = Param.String("64B", "ACP cache size")
  acpCacheLatency = Param.Int(1, "ACP cache tag latency")
  acpCacheMSHRs = Param.Int(4, "ACP cache number of MSHRs")

  enableStatsDump = Param.Bool(
      False, "Dump m5 stats after each accelerator invocation.")
  recordMemoryTrace = Param.Bool(
      False, "Record memory traffic going to/from the accelerator.")
  enableAcp = Param.Bool(
      False, "Connect the datapath's ACP port to the system L2.")
  useAcpCache = Param.Bool(
      True, "Use an L1 cache to handle ACP coherency traffic.")
  useAladdinDebugger = Param.Bool(
      False, "Start the Aladdin debugger on initialization.")

  spad_port = MasterPort("HybridDatapath DMA port")
  cache_port = MasterPort("HybridDatapath cache coherent port")
  acp_port = MasterPort("HybridDatapath ACP port")

  # HACK: We don't have a scratchpad object. Currently we just connect the
  # scratchpad port inside datapath directly to the memory bus.

  def connectThroughMonitor(self, monitor_name, master_port, slave_port):
    """ Connect the master and slave port through a CommMonitor. """
    trace_file_name = "%s.%s.gz" % (self.benchName, monitor_name)
    monitor = CommMonitor.CommMonitor()
    monitor.trace = MemTraceProbe.MemTraceProbe(trace_file=trace_file_name)
    monitor.slave = master_port
    monitor.master = slave_port
    setattr(self, monitor_name, monitor)

  def connectPrivateScratchpad(self, system, bus):
    if self.recordMemoryTrace:
      monitor_name = "spad_monitor"
      self.connectThroughMonitor(monitor_name, self.spad_port, bus.slave)
    else:
      self.spad_port = bus.slave

  def addPrivateL1Dcache(self, system, bus, dwc = None):
    self.cache_port = self.cache.cpu_side

    if self.recordMemoryTrace:
      monitor_name = "cache_monitor"
      self.connectThroughMonitor(monitor_name, self.cache.mem_side, bus.slave)
    else:
      self.cache.mem_side = bus.slave

  def connectAcpPort(self, tol2bus):
    if self.useAcpCache:
      self.acp_cache = AcpCache(size = self.acpCacheSize,
                                tag_latency = self.acpCacheLatency,
                                mshrs = self.acpCacheMSHRs,
                                write_buffers = self.acpCacheMSHRs)
      self.acp_port = self.acp_cache.cpu_side
      self.acp_cache.mem_side = tol2bus.slave
    else:
      self.acp_port = tol2bus.slave
