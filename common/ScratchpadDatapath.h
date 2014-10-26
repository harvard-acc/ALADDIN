#ifndef __SCRATCHPAD_DATAPATH__
#define __SCRATCHPAD_DATAPATH__

#include <string>

#include "BaseDatapath.h"
#include "Scratchpad.h"

class ScratchpadDatapath : public BaseDatapath {

  public:
    ScratchpadDatapath(std::string bench, std::string trace_file,
                       std::string config_file, float cycle_t);
    ~ScratchpadDatapath();

    void setScratchpad(Scratchpad *spad);
    void globalOptimizationPass();
    void initBaseAddress();
    void completePartition();
    void scratchpadPartition();
    virtual void stepExecutingQueue();
    virtual bool step();
    virtual void dumpStats();

  protected:
    Scratchpad *scratchpad;
};

#endif
