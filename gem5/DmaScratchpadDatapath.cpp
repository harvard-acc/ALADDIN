/* Implementation of an Aladdin accelerator with a datapath that is connected
 * to main memory via DMA.
 */

#include "base/types.hh"
#include "base/trace.hh"
#include "base/flags.hh"
#include "mem/request.hh"
#include "mem/packet.hh"
#include "mem/mem_object.hh"
#include "sim/system.hh"
#include "sim/sim_exit.hh"
#include "sim/clocked_object.hh"

#include "aladdin/common/ScratchpadDatapath.h"
#include "debug/DmaScratchpadDatapath.hh"
#include "DmaScratchpadDatapath.h"

DmaScratchpadDatapath::DmaScratchpadDatapath(
    const DmaScratchpadDatapathParams* params) :
    ScratchpadDatapath(params->benchName,
                       params->traceFileName,
                       params->configFileName,
                       params->cycleTime),
    MemObject(params),
    inFlightNodes(0),
    _dataMasterId(params->system->getMasterId(name() + ".dmadata")),
    spadPort(this, params->system),
    tickEvent(this),
    system(params->system)
{
  scratchpad = new Scratchpad(params->spadPorts);
  setGlobalGraph();
  initActualAddress();
  ScratchpadDatapath::globalOptimizationPass();
  setGraphForStepping();
  num_cycles = 0;
  system->registerAcceleratorStart();
  schedule(tickEvent, clockEdge(Cycles(1)));
}

DmaScratchpadDatapath::~DmaScratchpadDatapath()
{
  delete scratchpad;
}

// TODO: Copied from CacheDatapath.cpp. Some refactoring is in order.
void DmaScratchpadDatapath::initActualAddress()
{
  ostringstream file_name;
  file_name << benchName << "_memaddr.gz";
  gzFile gzip_file;
  gzip_file = gzopen(file_name.str().c_str(), "r");
  while (!gzeof(gzip_file))
  {
    char buffer[256];
    if (gzgets(gzip_file, buffer, 256) == NULL)
      break;
    unsigned node_id;
    long long int address;
    int size;
    sscanf(buffer, "%d,%lld,%d\n", &node_id, &address, &size);

    actualAddress[node_id] = make_pair(address & MASK, size/8);
  }
  gzclose(gzip_file);
}

void DmaScratchpadDatapath::event_step()
{
  step();
  scratchpad->step();
}

void
DmaScratchpadDatapath::stepExecutingQueue()
{
  auto it = executingQueue.begin();
  int index = 0;
  while (it != executingQueue.end())
  {
    unsigned node_id = *it;
    if (is_memory_op(microop.at(node_id)))
    {
      std::string node_part = baseAddress[node_id].first;
      if(scratchpad->canServicePartition(node_part))
      {
        assert(scratchpad->addressRequest(node_part));
        if (is_load_op(microop.at(node_id)))
          scratchpad->increment_loads(node_part);
        else
          scratchpad->increment_stores(node_part);
        /*
        executedNodes++;
        newLevel.at(node_id) = num_cycles;
        executingQueue.erase(it);
        updateChildren(node_id);
        it = executingQueue.begin();
        std::advance(it, index);
        */
        markNodeCompleted(it, index);
      }
      else
      {
        ++it;
        ++index;
      }
    }
    else if (is_dma_op(microop.at(node_id)))
    {
      DmaRequestStatus status = Ready;
      if (dma_requests.find(node_id) == dma_requests.end())
        dma_requests[node_id] = status;
      else
        status = dma_requests[node_id];
      if (status == Ready && inFlightNodes < MAX_INFLIGHT_NODES)
      {
        //first time see, do access
        Addr addr = actualAddress[node_id].first;
        int size = actualAddress[node_id].second / 16;
        bool isLoad = is_load_op(microop.at(node_id));
        issueDmaRequest(addr, size, isLoad, node_id);
        dma_requests[node_id] = Waiting;
        DPRINTF(DmaScratchpadDatapath, "node:%d is a dma request\n", node_id);
        ++it;
        ++index;
      }
      else if (status == Returned)
      {
        markNodeCompleted(it, index);
        dma_requests[*it] = status;
        inFlightNodes--;
      }
      else
      {
        // Still waiting from the cache. Move on to the next node.
        ++it;
        ++index;
      }
    }
    else
    {
      // Not a memory operation node, so it can be completed in one cycle.
      /*
      executedNodes++;
      newLevel.at(node_id) = num_cycles;
      executingQueue.erase(it);
      updateChildren(node_id);
      it = executingQueue.begin();
      std::advance(it, index);
      */
      markNodeCompleted(it, index);
    }
  }
}

bool
DmaScratchpadDatapath::step() {
  stepExecutingQueue();
  copyToExecutingQueue();
  num_cycles++;
  if (executedNodes < totalConnectedNodes)
  {
    schedule(tickEvent, clockEdge(Cycles(1)));
    return true;
  }
  else
  {
    dumpStats();
    system->registerAcceleratorExit();
    if (system->totalNumInsts == 0 &&  // no cpu
        system->numRunningAccelerators() == 0)
    {
      exitSimLoop("Aladdin called exit()");
    }
  }
  return false;
}

BaseMasterPort &
DmaScratchpadDatapath::getMasterPort(const string &if_name, PortID idx)
{
  // Get the right port based on name. This applies to all the
  // subclasses of the base CPU and relies on their implementation
  // of getDataPort and getInstPort. In all cases there methods
  // return a MasterPort pointer.
  if (if_name == "spad_port")
    return getDataPort();
  else
    return MemObject::getMasterPort(if_name);
}

/* Mark the DMA request node as having completed. */
void
DmaScratchpadDatapath::completeDmaAccess(PacketPtr pkt)
{
  DPRINTF(DmaScratchpadDatapath,
          "completeDmaAccess for addr:%#x %s\n",
          pkt->getAddr(),
          pkt->cmdString());
  DatapathSenderState *state = dynamic_cast<DatapathSenderState *> (pkt->senderState);

  //Mark nodes ready to fire
  unsigned node_id = state->node_id;
  assert(dma_requests.find(node_id) != dma_requests.end());
  assert(dma_requests[node_id] == Waiting);
  dma_requests[node_id] = Returned;
  DPRINTF(DmaScratchpadDatapath, "node:%d dma access is returned\n", node_id);
}

/* Issue a DMA request for memory. */
void DmaScratchpadDatapath::issueDmaRequest(
    Addr addr, unsigned size, bool isLoad, int node_id)
{
  DPRINTF(DmaScratchpadDatapath, "issueDmaRequest for addr:%#x\n", addr);
  MemCmd::Command cmd = isLoad ? MemCmd::ReadReq : MemCmd::WriteReq;
  Request::Flags flag = 0;
  // Should ultimately delay this by the scratchpad latency.
  spadPort.dmaAction(cmd, addr, size, NULL, NULL, clockEdge(Cycles(1)), flag);
}

bool DmaScratchpadDatapath::SpadPort::recvTimingResp(PacketPtr pkt)
{
  _datapath->completeDmaAccess(pkt);
  return DmaPort::recvTimingResp(pkt);
}

////////////////////////////////////////////////////////////////////////////
//
//  The SimObjects we use to get the Datapath information into the simulator
//
////////////////////////////////////////////////////////////////////////////

DmaScratchpadDatapath* DmaScratchpadDatapathParams::create()
{
  return new DmaScratchpadDatapath(this);
}
