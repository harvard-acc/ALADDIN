#ifndef _DEBUG_PRINTER_H_
#define _DEBUG_PRINTER_H_

#include <iostream>

#include "ExecNode.h"
#include "ScratchpadDatapath.h"
#include "SourceManager.h"

class DebugNodePrinter {
 public:
  DebugNodePrinter(ExecNode* _node,
                   ScratchpadDatapath* _acc,
                   std::ostream& _out)
      : node(_node), acc(_acc), out(_out),
        srcManager(acc->get_source_manager()) {}

  void printAll();
  void printBasic();
  void printSourceInfo();
  void printMemoryOp();
  void printGep();
  void printCall();
  void printChildren();
  void printParents();
  void printExecutionStats();

 private:
  ExecNode* node;
  ScratchpadDatapath* acc;
  std::ostream& out;
  SrcTypes::SourceManager& srcManager;
};

class DebugLoopPrinter {
 public:
  DebugLoopPrinter(ScratchpadDatapath* _acc, std::ostream& _out)
      : acc(_acc), out(_out), srcManager(acc->get_source_manager()) {}

  void printLoop(const std::string& loop_name);
  void printAllLoops();

 private:

  enum LoopIdentifyStatus {
    LOOP_FOUND,
    LOOP_NOT_FOUND,
    ABORTED,
  };
  enum LoopExecutionStatus {
    NOT_STARTED,
    STARTED,

  };

  /* Identify the UniqueLabel referred to by loop_name.
   *
   * If there are multiple labeled statements by that name, prompt the user for
   * a selection.
   */
  LoopIdentifyStatus identifyLoop(const std::string& loop_name);

  /* Get a numeric selection from the user, from 1 to max_option. */
  int getUserSelection(int max_option);

  /* Return pairs of nodes that demarcate the start and end of a loop boundary.
   *
   * The loop is identified by the class member selected_label.
   */
  std::list<node_pair_t> findLoopBoundaries();

  /* Compute the max latency of the loops bounded by loop_bound_nodes.
   *
   * loop_bound_nodes is the result of findLoopBoundaries().
   */
  int computeLoopLatency(const std::list<node_pair_t>& loop_bound_nodes);

  /* The loop to print. */
  SrcTypes::UniqueLabel selected_label;
  ScratchpadDatapath* acc;
  std::ostream& out;
  SrcTypes::SourceManager& srcManager;
};

#endif
