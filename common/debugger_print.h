#ifndef _DEBUG_PRINTER_H_
#define _DEBUG_PRINTER_H_

#include <iostream>

#include "ExecNode.h"
#include "ScratchpadDatapath.h"
#include "SourceManager.h"

class DebugPrinter {
 public:
  DebugPrinter(ExecNode* _node, ScratchpadDatapath* _acc, std::ostream& _out)
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

#endif
