#ifndef _DEBUG_PRINTER_H_
#define _DEBUG_PRINTER_H_

#include <iostream>

#include "../ExecNode.h"
#include "../ScratchpadDatapath.h"
#include "../SourceManager.h"

namespace adb {

// Formatted print out of pairs of node ids.
//
// Prints up up to 80 pairs in rows of 4 columns each.
void print_node_pair_list(std::list<node_pair_t> pairs,
                          std::string row_header,
                          std::ostream& out);

class DebugPrinterBase {
 public:
  DebugPrinterBase(ScratchpadDatapath* _acc, std::ostream& _out)
      : acc(_acc), prog(_acc->getProgram()),
        srcManager(acc->get_source_manager()), out(_out) {}
  virtual ~DebugPrinterBase() = 0;

 protected:
  ScratchpadDatapath* acc;
  const Program& prog;
  SrcTypes::SourceManager& srcManager;
  std::ostream& out;
};

class DebugNodePrinter : public DebugPrinterBase {
 public:
  DebugNodePrinter(const ExecNode* _node,
                   ScratchpadDatapath* _acc,
                   std::ostream& _out)
      : DebugPrinterBase(_acc, _out), node(_node) {}

  void printAll();
  void printBasic();
  void printSourceInfo();
  void printMemoryOp();
  void printDmaOp();
  void printGep();
  void printCall();
  void printChildren();
  void printParents();
  void printExecutionStats();

 private:
  const ExecNode* node;
};

class DebugEdgePrinter : public DebugPrinterBase {
 public:
  DebugEdgePrinter(const ExecNode* _source_node,
                   const ExecNode* _target_node,
                   ScratchpadDatapath* _acc,
                   std::ostream& _out)
      : DebugPrinterBase(_acc, _out), source_node(_source_node),
        target_node(_target_node) {}

  void printAll();
  void printSourceInfo();
  void printTargetInfo();
  void printEdgeInfo();

 private:
  const ExecNode* source_node;
  const ExecNode* target_node;
};

class DebugLoopPrinter : public DebugPrinterBase {
 public:
  DebugLoopPrinter(ScratchpadDatapath* _acc, std::ostream& _out)
      : DebugPrinterBase(_acc, _out) {}

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
  int computeLoopLatency(const std::list<cnode_pair_t>& loop_bound_nodes);

  /* The loop to print. */
  SrcTypes::UniqueLabel selected_label;
};

class DebugFunctionPrinter : public DebugPrinterBase {
 public:
  DebugFunctionPrinter(std::string _function_name,
                       ScratchpadDatapath* _acc,
                       std::ostream& _out)
      : DebugPrinterBase(_acc, _out) {
    function = srcManager.get<SrcTypes::Function>(_function_name);
    function_boundaries = acc->getProgram().findFunctionBoundaries(function);
  }

  void printAll();
  void printBasic();
  void printFunctionBoundaries();
  void printLoops();
  void printExecutionStats();
  int computeFunctionLatency();

 private:
  SrcTypes::Function* function;
  std::list<cnode_pair_t> function_boundaries;
};

class DebugCyclePrinter : public DebugPrinterBase {
 public:
  DebugCyclePrinter(int _cycle,
                    unsigned _max_nodes,
                    ScratchpadDatapath* _acc,
                    std::ostream& _out)
      : DebugPrinterBase(_acc, _out), cycle(_cycle), max_nodes(_max_nodes) {}

  void printAll();
  std::list<const ExecNode*> findNodesExecutedinCycle();

 private:
  int cycle;
  unsigned max_nodes;
};

};  // namespace adb

#endif
