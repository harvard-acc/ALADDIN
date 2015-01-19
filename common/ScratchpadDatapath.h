#ifndef __SCRATCHPAD_DATAPATH__
#define __SCRATCHPAD_DATAPATH__

#include <string>

#include "BaseDatapath.h"
#include "Scratchpad.h"

class ScratchpadDatapath : public BaseDatapath {

  public:
    ScratchpadDatapath(std::string bench, std::string trace_file,
                       std::string config_file);
    ~ScratchpadDatapath();

    void globalOptimizationPass();
    void completePartition();
    void scratchpadPartition();
    virtual void initBaseAddress();
    virtual void stepExecutingQueue();
    virtual bool step();
    virtual void dumpStats();
    virtual double getTotalMemArea();
    virtual unsigned getTotalMemSize();
    virtual void getAverageMemPower(
        unsigned int cycles, float *avg_power,
        float *avg_dynamic, float *avg_leak);
    virtual void getMemoryBlocks(std::vector<std::string> &names);

  protected:
    Scratchpad *scratchpad;
};

#endif
