#ifndef _DEBUG_PRINTER_H_
#define _DEBUG_PRINTER_H_

#include <iostream>

#include "ExecNode.h"
#include "ScratchpadDatapath.h"
#include "SourceManager.h"

// Formatted print out of pairs of node ids.
//
// Prints up up to 80 pairs in rows of 4 columns each.
void print_node_pair_list(std::list<node_pair_t> pairs,
                          std::string row_header,
                          std::ostream& out);

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

class DebugEdgePrinter {
 public:
  DebugEdgePrinter(ExecNode* _source_node,
                   ExecNode* _target_node,
                   ScratchpadDatapath* _acc,
                   std::ostream& _out)
      : source_node(_source_node), target_node(_target_node), acc(_acc),
        out(_out), srcManager(acc->get_source_manager()) {}

  void printAll();
  void printSourceInfo();
  void printTargetInfo();
  void printEdgeInfo();

 private:
  ExecNode* source_node;
  ExecNode* target_node;
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

  /* Compute the max latency of the loops bounded by loop_bound_nodes.
   */
  int computeLoopLatency(const std::list<node_pair_t>& loop_bound_nodes);

  /* The loop to print. */
  SrcTypes::UniqueLabel selected_label;
  ScratchpadDatapath* acc;
  std::ostream& out;
  SrcTypes::SourceManager& srcManager;
};

class DebugFunctionPrinter {
 public:
  DebugFunctionPrinter(std::string _function_name,
                       ScratchpadDatapath* _acc,
                       std::ostream& _out)
      : acc(_acc), out(_out), srcManager(acc->get_source_manager()) {
    function = srcManager.get<SrcTypes::Function>(_function_name);
    function_boundaries = acc->findFunctionBoundaries(function);
  }

  void printAll();
  void printBasic();
  void printFunctionBoundaries();
  void printLoops();
  void printExecutionStats();
  int computeFunctionLatency();

 private:
  SrcTypes::Function* function;
  std::list<node_pair_t> function_boundaries;
  ScratchpadDatapath* acc;
  std::ostream& out;
  SrcTypes::SourceManager& srcManager;
};

#endif
