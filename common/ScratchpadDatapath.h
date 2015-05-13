#ifndef __SCRATCHPAD_DATAPATH__
#define __SCRATCHPAD_DATAPATH__

#include <string>

#include "BaseDatapath.h"
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
  virtual void updateChildren(unsigned node_id);
  virtual void initBaseAddress();
  virtual void stepExecutingQueue();
  virtual bool step();
  virtual void dumpStats();
  virtual double getTotalMemArea();
  virtual unsigned getTotalMemSize();
  virtual void getAverageMemPower(unsigned int cycles,
                                  float* avg_power,
                                  float* avg_dynamic,
                                  float* avg_leak);
  virtual void getMemoryBlocks(std::vector<std::string>& names);
  virtual int rescheduleNodesWhenNeeded();
  /* Access cacti_interface() to calculate power/area. */
  uca_org_t cactiWrapper(unsigned num_of_bytes, unsigned wordsize);

 protected:
  Scratchpad* scratchpad;
  /*True if any of the scratchpads can still service memory requests.
    False if non of the scratchpads can service any memory requests.*/
  bool scratchpadCanService;
  /*An array to keep track of time before each node executes*/
  std::vector<float> timeBeforeNodeExecution;
};

#endif
