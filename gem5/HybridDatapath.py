from m5.params import *
from MemObject import MemObject
from m5.proxy import *

class HybridDatapath(MemObject):
  type = "HybridDatapath"
  cxx_header = "aladdin/gem5/HybridDatapath.h"
  benchName = Param.String("Aladdin Bench Name")
  traceFileName = Param.String("Aladdin Input Trace File")
  configFileName = Param.String("Aladdin Config File")
  cycleTime = Param.Unsigned(1, "Clock Period: 1ns default")
  acceleratorName = Param.String("", "Unique accelerator name")
  acceleratorId = Param.Int(-1, "Accelerator Id")
  experimentName = Param.String("NULL", "Experiment name. String identifier "
      "for a set of related simulations.")
  system = Param.System(Parent.any, "system object")
  executeStandalone = Param.Bool(True, "Execute Aladdin standalone, without a "
      "CPU/user program.")
  useDb = Param.Bool(False, "Store results in database.")

  # Scratchpad/DMA parameters.
  dmaSetupLatency = Param.Unsigned(1000, "DMA Setup Latency")
  maxDmaRequests = Param.Unsigned(16, "Max number of outstanding DMA requests")
  spadPorts = Param.Unsigned(1, "Scratchpad ports per partition")

  # Cache parameters.
  cacheSize = Param.String("16kB", "Private cache size")
  cacheLineSize = Param.Int("64", "Cache line size (in bytes)")
  cacheAssoc = Param.Int(1, "Private cache associativity")
  l2cacheSize = Param.String("128kB", "Shared L2 cache size")
  cacheHitLatency = Param.Int(1, "Hit latency")
  cactiCacheConfig = Param.String("", "CACTI cache config file")
  tlbEntries = Param.Int(0, "number entries in TLB (0 implies infinite)")
  tlbAssoc = Param.Int(4, "Number of sets in the TLB")
  tlbHitLatency = Param.Cycles(0, "number of cycles for a hit")
  tlbMissLatency = Param.Cycles(10, "number of cycles for a miss")
  tlbPageBytes = Param.Int(4096, "Page Size")
  tlbCactiConfig = Param.String("", "TLB CACTI configuration file.")
  isPerfectTLB = Param.Bool(False, "Is this TLB perfect (e.g. always hit)")
  numOutStandingWalks = Param.Int(4, "num of outstanding page walks")
  loadQueueSize = Param.Int(16, "Size of the load queue.")
  loadBandwidth = Param.Int(
      1, "Number of loads that can be issued per cycle.")
  loadQueueCacheConfig = Param.String("", "Load queue CACTI config file.")
  storeQueueSize = Param.Int(16, "Size of the store queue.")
  storeBandwidth = Param.Int(
      1, "Number of stores that can be issued per cycle.")
  storeQueueCacheConfig = Param.String("", "Store queue CACTI config file.")
  tlbBandwidth = Param.Int(
      1, "Number of translations that can be requested per cycle.")

  spad_port = MasterPort("HybridDatapath DMA port")
  cache_port = MasterPort("HybridDatapath cache coherent port")
  # This is the set of ports that should be connected to the memory bus.
  _cached_ports = ['spad_port', "cache_port"]
  _uncached_slave_ports = []
  _uncached_master_ports = []

  def connectCachedPorts(self, bus):
      for p in self._cached_ports:
          exec('self.%s = bus.slave' % p)

  def connectUncachedPorts(self, bus):
      for p in self._uncached_slave_ports:
          exec('self.%s = bus.master' % p)
      for p in self._uncached_master_ports:
          exec('self.%s = bus.slave' % p)

  def connectAllPorts(self, cached_bus, uncached_bus = None) :
    self.connectCachedPorts(cached_bus)
    if not uncached_bus:
      uncached_bus = cached_bus
    self.connectUncachedPorts(uncached_bus)

  def addPrivateScratchpad(self, spad, dwc = None) :
    self.spad = spad
    self.spad_port = spad.cpu_side
    # The spad port is now connected to the scratchpad itself, so it should not
    # be reconnected to the memory bus later.
    self._cached_ports.remove("spad_port")
    self._cached_ports.append('spad.mem_side')

  def addPrivateL1Dcache(self, cache, dwc = None) :
    self.cache = cache
    self.cache_port = cache.cpu_side
    self._cached_ports.remove("cache_port")
    self._cached_ports.append('cache.mem_side')
