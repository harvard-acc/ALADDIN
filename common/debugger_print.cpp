#include <exception>
#include <iostream>
#include <map>
#include <vector>

#include "ExecNode.h"
#include "ScratchpadDatapath.h"
#include "SourceManager.h"
#include "SourceEntity.h"

#include "debugger_print.h"

void DebugPrinter::printAll() {
  printBasic();
  printSourceInfo();
  printMemoryOp();
  printGep();
  printCall();
  printParents();
  printChildren();
}

void DebugPrinter::printBasic() {
  out << "Node " << node->get_node_id() << ":\n"
      << "  Opcode: " << node->get_microop_name() << "\n";
  out << "  Inductive: ";
  if (node->is_inductive())
    out << "Yes\n";
  else
    out << "No\n";
}

void DebugPrinter::printSourceInfo() {
  using namespace SrcTypes;

  SourceManager& srcManager = acc->get_source_manager();
  int line_num = node->get_line_num();
  if (line_num != -1)
    out << "  Line number: " << line_num << "\n";

  // What function does this node belong to?
  Function func = srcManager.get<Function>(node->get_static_function_id());
  if (func != InvalidFunction) {
    out << "  Parent function: " << func.get_name() << "\n";
    out << "  Dynamic invocation count: " << node->get_dynamic_invocation()
        << "\n";
  }

  if (line_num != -1 && func != InvalidFunction) {
    const std::multimap<unsigned, UniqueLabel>& labelmap = acc->getLabelMap();
    auto label_range = labelmap.equal_range(line_num);
    for (auto it = label_range.first; it != label_range.second; ++it) {
      if (it->second.get_function_id() == func.get_id()) {
        src_id_t label_id = it->second.get_label_id();
        Label label = srcManager.get<Label>(label_id);
        out << "  Code label: " << label.get_name() << "\n";
        break;
      }
    }
  }

  // Print information about the variables involved, if info exists.
  Variable var = srcManager.get<Variable>(node->get_variable_id());
  if (var != InvalidVariable)
    out << "  Variable: " << var.str() << "\n";
}

void DebugPrinter::printMemoryOp() {
  if (node->is_memory_op()) {
    MemAccess* mem_access = node->get_mem_access();
    out << "  Memory access to array " << node->get_array_label() << "\n"
        << "    Addr:  0x" << std::hex << mem_access->vaddr << "\n"
        << "    Size:  " << std::dec << mem_access->size << "\n";
    out << "    Value: ";
    if (mem_access->is_float && mem_access->size == 4)
      out << FP2BitsConverter::ConvertBitsToFloat(mem_access->value);
    else if (mem_access->is_float && mem_access->size == 8)
      out << FP2BitsConverter::ConvertBitsToDouble(mem_access->value);
    else
      out << mem_access->value;
    out << "\n";
  }
}

void DebugPrinter::printGep() {
  if (node->get_microop() == LLVM_IR_GetElementPtr) {
    out << "  GEP of array " << node->get_array_label() << "\n";
  }
}

void DebugPrinter::printCall() {
  using namespace SrcTypes;

  // For Call nodes, the next node will indicate what the called function is
  // (unless the called function is empty).
  if (node->is_call_op()) {
    ExecNode* called_node = NULL;
    try {
      called_node = acc->getNodeFromNodeId(node->get_node_id() + 1);
    } catch (const std::out_of_range& e) {
    }

    Function curr_func =
        srcManager.get<Function>(node->get_static_function_id());
    out << "  Called function: ";
    if (called_node) {
      Function called_func =
          srcManager.get<Function>(called_node->get_static_function_id());
      if (called_func != curr_func && called_func != InvalidFunction)
        out << called_func.get_name() << "\n";
      else
        out << "Unable to determine.\n";
    } else {
      out << "Unable to determine.\n";
    }
  }
}

void DebugPrinter::printChildren() {
  std::vector<unsigned> childNodes = acc->getChildNodes(node->get_node_id());
  out << "  Children: " << childNodes.size() << " [ ";
  for (auto child_node : childNodes) {
    out << child_node << " ";
  }
  out << "]\n";
}

void DebugPrinter::printParents() {
  std::vector<unsigned> parentNodes = acc->getParentNodes(node->get_node_id());
  out << "  Parents: " << parentNodes.size() << " [ ";
  for (auto parent_node : parentNodes) {
    out << parent_node << " ";
  }
  out << "]\n";
}
