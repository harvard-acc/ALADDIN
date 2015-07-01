#ifndef __SCRATCHPAD_DATAPATH__
#define __SCRATCHPAD_DATAPATH__

#include <string>

#include "BaseDatapath.h"
#include "DatabaseDeps.h"
#include "ExecNode.h"
#include "Scratchpad.h"

class ScratchpadDatapath : public BaseDatapath {

 public:
  ScratchpadDatapath(std::string bench,
                     std::string trace_file,
                     std::string config_file,
                     int num_spad_ports = 1);
  virtual ~ScratchpadDatapath();

  void globalOptimizationPass();
  void completePartition();
  void scratchpadPartition();
  virtual void clearDatapath();
  virtual void initBaseAddress();
  virtual void stepExecutingQueue();
  virtual bool step();
  virtual double getTotalMemArea();
  virtual void getAverageMemPower(unsigned int cycles,
                                  float* avg_power,
                                  float* avg_dynamic,
                                  float* avg_leak);
  virtual void getMemoryBlocks(std::vector<std::string>& names);
  virtual void updateChildren(ExecNode* node);
  virtual int rescheduleNodesWhenNeeded();

 protected:
  Scratchpad* scratchpad;
  /*True if any of the scratchpads can still service memory requests.
    False if non of the scratchpads can service any memory requests.*/
  bool scratchpadCanService;
  /* Stores number of cycles left for multi-cycle nodes currently in flight. */
  std::map<unsigned, unsigned> inflight_multicycle_nodes;
#ifdef USE_DB
  int writeConfiguration(sql::Connection* con);
#endif
};

#endif
