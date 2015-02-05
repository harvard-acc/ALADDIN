from m5.params import *
from MemObject import MemObject
from m5.proxy import *

class DmaScratchpadDatapath(MemObject):
  type = "DmaScratchpadDatapath"
  cxx_header = "aladdin/gem5/DmaScratchpadDatapath.h"
  benchName = Param.String("Aladdin Bench Name")
  traceFileName = Param.String("Aladdin Input Trace File")
  configFileName = Param.String("Aladdin Config File")
  cycleTime = Param.Unsigned(1, "Clock Period: 1ns default")
  acceleratorId = Param.Int(-1, "Accelerator Id")
  acceleratorDeps = Param.String("", "Accelerator dependencies.")
  # Number of cycles required for the CPU to reinitiate a DMA transfer.
  dmaSetupLatency = Param.Unsigned(1000, "DMA Setup Latency")
  maxDmaRequests = Param.Unsigned(16, "Max number of outstanding DMA requests")
  spadPorts = Param.Unsigned(1, "Scratchpad ports per partition")
  useDb = Param.Bool(False, "Store results in database.")
  experimentName = Param.String("NULL", "Experiment name. String identifier "
      "for a set of related simulations.")
  system = Param.System(Parent.any, "system object")
  executeStandalone = Param.Bool(True, "Execute Aladdin standalone, without a "
      "CPU/user program.")

  spad_port = MasterPort("DmaScratchpadDatapath data port")
  _cached_ports = ['spad_port']
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
    self._cached_ports = ['spad.mem_side']
