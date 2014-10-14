#ifndef __SCRATCHPAD_DATAPATH__
#define __SCRATCHPAD_DATAPATH__

#include <string>

#include "BaseDatapath.h"
#include "Scratchpad.h"

class ScratchpadDatapath : public BaseDatapath {

  public:
    ScratchpadDatapath(std::string bench, float cycle_t);
    ~ScratchpadDatapath();

    void globalOptimizationPass();
    void completePartition();
    void initBaseAddress();
    void scratchpadPartition();
    void setScratchpad(Scratchpad *spad);
    bool step();
    void stepExecutingQueue();
    void dumpStats();

  private:
    Scratchpad *scratchpad;
};

#endif
