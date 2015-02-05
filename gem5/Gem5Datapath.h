#ifndef __GEM5_DATAPATH_H__
#define __GEM5_DATAPATH_H__

#include "mem/mem_object.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"
#include "sim/system.hh"

#include "aladdin/common/BaseDatapath.h"

/* A collection of functions common to datapath objects used within GEM5.
 * Note: to avoid potential problems arising from the use of multiple
 * inheritance with this integration, Gem5Datapath should never extend
 * BaseDatapath.
 */
class Gem5Datapath : public MemObject
{

  public:
    /* These extra parameters are needed because making a Gem5DatapathParams
     * object that serves as a base class to CacheDatapathParams and
     * DmaScratchpadDatapathParams is more work than it is worth, and
     * Gem5Datapath has to pass the base class of MemObjectParams to its parent
     * constructor, which doesn't contain any of our custom parameters.
     */
    Gem5Datapath(const MemObjectParams* params,
                 int _accelerator_id,
                 bool _execute_standalone,
                 System* _system) :
        MemObject(params),
        execute_standalone(_execute_standalone),
        accelerator_id(_accelerator_id),
        system(_system) {}

    /* Return the tick event object for the event queue for this datapath. */
    virtual Event& getTickEvent() = 0;

    // Add the tick event to the gem5 event queue.
    void scheduleOnEventQueue(unsigned delay_cycles = 1)
    {
      schedule(getTickEvent(), clockEdge(Cycles(delay_cycles)));
    }

  protected:
    // True if gem5 is being simulated with just Aladdin, false if there are
    // CPUs in the system that are executing code and manually invoking the
    // accelerators.
    bool execute_standalone;

    // Accelerator id, assigned by the system. It can also be the ioctl request
    // code for the particular kernel.
    int accelerator_id;

    // Dependencies of this accelerator, expressed as a list of other
    // accelerator ids. All other accelerators must complete execution before
    // this one can be scheduled.
    std::vector<int> accelerator_deps;

    // Pointer to the rest of the system.
    System* system;

};

#endif
