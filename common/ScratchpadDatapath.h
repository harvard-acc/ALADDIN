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
                     std::string config_file);
  ~ScratchpadDatapath();

  void globalOptimizationPass();
  void completePartition();
  void scratchpadPartition();
  virtual void setGraphForStepping();
  virtual void updateChildren(ExecNode* node);
  virtual void initBaseAddress();
  virtual void stepExecutingQueue();
  virtual bool step();
  virtual void dumpStats();
  virtual double getTotalMemArea();
  virtual void getAverageMemPower(unsigned int cycles,
                                  float* avg_power,
                                  float* avg_dynamic,
                                  float* avg_leak);
  virtual void getMemoryBlocks(std::vector<std::string>& names);

 protected:
  Scratchpad* scratchpad;
  /*True if any of the scratchpads can still service memory requests.
    False if non of the scratchpads can service any memory requests.*/
  bool scratchpadCanService;
#ifdef USE_DB
  int writeConfiguration(sql::Connection* con);
#endif
};

#endif
