#ifndef __SCRATCHPAD_DATAPATH__
#define __SCRATCHPAD_DATAPATH__

#include <string>

#include "BaseDatapath.h"
#include "Scratchpad.h"

class ScratchpadDatapath : public BaseDatapath {

  public:
    ScratchpadDatapath(std::string bench, std::string trace_file, std::string config_file, float cycle_t);
    ~ScratchpadDatapath();

    void setScratchpad(Scratchpad *spad);
    void globalOptimizationPass();
    bool step();
    void dumpStats();
    void initBaseAddress();
    void completePartition();
    void scratchpadPartition();
    void stepExecutingQueue();

  private:

    Scratchpad *scratchpad;
};

#endif
