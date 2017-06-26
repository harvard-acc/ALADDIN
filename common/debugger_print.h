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

  LoopIdentifyStatus identifyLoop(const std::string& loop_name);
  int getUserSelection(int max_option);

  // Pair of UniqueLabel and the line number.
  std::pair<SrcTypes::UniqueLabel, unsigned> selected_label;
  ScratchpadDatapath* acc;
  std::ostream& out;
  SrcTypes::SourceManager& srcManager;
};

#endif
