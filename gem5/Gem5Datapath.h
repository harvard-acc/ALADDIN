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
class Gem5Datapath : public MemObject {

 public:
  /* These extra parameters are needed because making a Gem5DatapathParams
   * object that serves as a base class to HybridDatapathParams.
   * Gem5Datapath has to pass the base class of MemObjectParams to its parent
   * constructor, which doesn't contain any of our custom parameters.
   */
  Gem5Datapath(const MemObjectParams* params,
               int _accelerator_id,
               bool _execute_standalone,
               System* _system)
      : MemObject(params), execute_standalone(_execute_standalone),
        accelerator_id(_accelerator_id), finish_flag(0), context_id(-1),
        thread_id(-1), cycles_since_last_node(0), system(_system) {}

  /* Return the tick event object for the event queue for this datapath. */
  virtual Event& getTickEvent() = 0;

  /* Build, optimize, register and prepare datapath for scheduling. */
  virtual void initializeDatapath(int delay = 1) = 0;

  /* Add the tick event to the gem5 event queue. */
  void scheduleOnEventQueue(unsigned delay_cycles = 1) {
    schedule(getTickEvent(), clockEdge(Cycles(delay_cycles)));
  }

  /* Set the accelerator's context ids to be that of the invoking thread. */
  void setContextThreadIds(int _context_id, int _thread_id) {
    context_id = _context_id;
    thread_id = _thread_id;
  }

  void setFinishFlag(Addr _finish_flag) { finish_flag = _finish_flag; }

  /* True if there are no CPUs in the simulation, false otherwise. */
  bool isExecuteStandalone() { return execute_standalone; }

  /* Send a signal to the rest of the system that the accelerator has
   * finished. This signal takes the form of a shared memory block. This is
   * used by user level programs that invoke Aladdin during program execution
   * and spin while waiting for the accelerator to complete.  Different
   * datapath implementations will need different mechanisms to accomplish
   * this.
   */
  virtual void sendFinishedSignal() = 0;

  /* Return the base address of the array specified by label. The base address
   * corresponds to the zeroth element of the array.
   */
  virtual Addr getBaseAddress(std::string label) = 0;

  /* Insert a vpn -> ppn mapping into the datapath's TLB.
   *
   * This is currently used to create a mapping between the simulated virtual
   * address of an array in the program with the actual physical address in
   * the simulator.
   *
   * Datapath models that do not support TLB structures should abort
   * simulation if this is called.
   *
   * TODO(samxi): Maybe this should not be part of Gem5Datapath? But I'm not
   * sure if a dynamic_cast to HybridDatapath would work here.
   */
  virtual void insertTLBEntry(Addr vaddr, Addr paddr) = 0;

  /* Insert a mapping between array labels to their simulated virtual
   * addresses. */
  virtual void insertArrayLabelToVirtual(std::string array_label,
                                         Addr vaddr) = 0;

 protected:
  /* True if gem5 is being simulated with just Aladdin, false if there are
   * CPUs in the system that are executing code and manually invoking the
   * accelerators.
   */
  bool execute_standalone;

  /* Accelerator id, assigned by the system. It can also be the ioctl request
   * code for the particular kernel.
   */
  int accelerator_id;

  /* The accelerator will call sendFinishedSignal on this address to signal
   * its completion to the CPU. This is a physical address.
   */
  Addr finish_flag;

  /* Thread and context ids of the thread that activated this accelerator. */
  int context_id;
  int thread_id;

  /* Deadlock detector. Counts the number of ticks that have passed since the
   * last node was executed. If this exceeds deadlock_threshold, then
   * simulation is terminated with an error. */
  unsigned cycles_since_last_node;
  const unsigned deadlock_threshold = 1000000;

  /* Dependencies of this accelerator, expressed as a list of other
   * accelerator ids. All other accelerators must complete execution before
   * this one can be scheduled.
   */
  std::vector<int> accelerator_deps;

  /* Pointer to the rest of the system. */
  System* system;
};

#endif
