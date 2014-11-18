from m5.params import *
from MemObject import MemObject
from m5.proxy import *

class CacheDatapath(MemObject):
  type = 'CacheDatapath'
  cxx_header = "aladdin/gem5/CacheDatapath.h"
  benchName = Param.String("Aladdin Bench Name")
  traceFileName = Param.String("Aladdin Input Trace File")
  configFileName = Param.String("Aladdin Config File")
  cycleTime = Param.Unsigned(6, "Clock Period: 6ns default")

  cacheSize = Param.String("16kB", "Private cache size")
  cacheAssoc = Param.Int(1, "Private cache associativity")
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

  system = Param.System(Parent.any, "system object")

  dcache_port = MasterPort("CacheDatapath Data Port")
  _cached_ports = ['dcache_port']
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

  def addPrivateL1Dcache(self, dc, dwc = None) :
    self.dcache = dc
    self.dcache_port = dc.cpu_side
    self._cached_ports = ['dcache.mem_side']

    #if dwc :
      #self.dtb_walker_cache = dwc
      #self.dtb.walker.port = dwc.cpu_side
      #self._cached_ports += ["dtb_walker_cache.mem_side"]
    #else:
      #self._cached_ports += ["dtb.walker.port"]

