#ifndef __SCRATCHPAD_DATAPATH__
#define __SCRATCHPAD_DATAPATH__

#include <string>

#include "BaseDatapath.h"
#include "ExecNode.h"
#include "Scratchpad.h"

class ScratchpadDatapath : public BaseDatapath {

 public:
  ScratchpadDatapath(std::string bench,
                     std::string trace_file,
                     std::string config_file);
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
  virtual void writeOtherStats();
  Scratchpad* scratchpad;
  /*True if any of the scratchpads can still service memory requests.
    False if non of the scratchpads can service any memory requests.*/
  bool scratchpadCanService;
  /* Stores number of cycles left for multi-cycle nodes currently in flight. */
  std::map<unsigned, unsigned> inflight_multicycle_nodes;
  // To streamline code.
  const float cycle_time;

  // If we invoke the same accelerator more than once, we don't need to create
  // scratchpad objects again.  (the scratchpads can't change), but we will
  // need to assign labels and partitions to the nodes.
  bool mem_reg_conversion_executed;
  bool scratchpad_partition_executed;
};

#endif
