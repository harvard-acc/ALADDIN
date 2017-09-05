#include <exception>
#include <iostream>
#include <map>
#include <vector>

#ifdef HAS_READLINE
#include <readline/readline.h>
#endif

#include <boost/algorithm/string.hpp>

#include "ExecNode.h"
#include "ScratchpadDatapath.h"
#include "SourceManager.h"
#include "SourceEntity.h"

#include "debugger.h"
#include "debugger_print.h"

//-------------------
// DebugNodePrinter
//-------------------

void DebugNodePrinter::printAll() {
  printBasic();
  printSourceInfo();
  printMemoryOp();
  printGep();
  printCall();
  printParents();
  printChildren();
  printExecutionStats();
}

void DebugNodePrinter::printBasic() {
  out << "Node " << node->get_node_id() << ":\n"
      << "  Opcode: " << node->get_microop_name() << "\n";
  out << "  Inductive: ";
  if (node->is_inductive())
    out << "Yes\n";
  else
    out << "No\n";
}

void DebugNodePrinter::printSourceInfo() {
  using namespace SrcTypes;

  SourceManager& srcManager = acc->get_source_manager();
  int line_num = node->get_line_num();
  if (line_num != -1)
    out << "  Line number: " << line_num << "\n";

  // What function does this node belong to?
  SrcTypes::Function* func = node->get_static_function();
  if (func) {
    out << "  Parent function: " << func->get_name() << "\n";
    out << "  Dynamic invocation count: " << node->get_dynamic_invocation()
        << "\n";
  }

  if (line_num != -1 && func) {
    const std::multimap<unsigned, UniqueLabel>& labelmap =
        acc->getLabelMap();
    auto label_range = labelmap.equal_range(line_num);
    for (auto it = label_range.first; it != label_range.second; ++it) {
      if (it->second.get_function() == func) {
        Label* label = it->second.get_label();
        out << "  Code label: " << label->get_name() << "\n";
        break;
      }
    }
  }

  // Print information about the variables involved, if info exists.
  Variable* var = node->get_variable();
  if (var)
    out << "  Variable: " << var->str() << "\n";
}

void DebugNodePrinter::printMemoryOp() {
  if (node->is_memory_op()) {
    MemAccess* mem_access = node->get_mem_access();
    out << "  Memory access to array " << node->get_array_label() << "\n"
        << "    Addr:  0x" << std::hex << mem_access->vaddr << "\n"
        << "    Partition: " << node->get_partition_index() << "\n"
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

void DebugNodePrinter::printGep() {
  if (node->get_microop() == LLVM_IR_GetElementPtr) {
    out << "  GEP of array " << node->get_array_label() << "\n";
  }
}

void DebugNodePrinter::printCall() {
  using namespace SrcTypes;

  // For Call nodes, the next node will indicate what the called function is
  // (unless the called function is empty).
  if (node->is_call_op()) {
    ExecNode* called_node = NULL;
    try {
      called_node = acc->getNodeFromNodeId(node->get_node_id() + 1);
    } catch (const std::out_of_range& e) {
    }

    SrcTypes::Function* curr_func = node->get_static_function();
    out << "  Called function: ";
    if (called_node) {
      SrcTypes::Function* called_func = called_node->get_static_function();
      if (called_func && called_func != curr_func)
        out << called_func->get_name() << "\n";
      else
        out << "Unable to determine.\n";
    } else {
      out << "Unable to determine.\n";
    }
  }
}

void DebugNodePrinter::printChildren() {
  std::vector<unsigned> childNodes = acc->getChildNodes(node->get_node_id());
  out << "  Children: " << childNodes.size() << " [ ";
  for (auto child_node : childNodes) {
    ExecNode* node = acc->getNodeFromNodeId(child_node);
    if (!node->is_isolated())
      out << child_node << " ";
  }
  out << "]\n";
}

void DebugNodePrinter::printParents() {
  std::vector<unsigned> parentNodes = acc->getParentNodes(node->get_node_id());
  out << "  Parents: " << parentNodes.size() << " [ ";
  for (auto parent_node : parentNodes) {
    ExecNode* node = acc->getNodeFromNodeId(parent_node);
    if (!node->is_isolated())
      out << parent_node << " ";
  }
  out << "]\n";
}

void DebugNodePrinter::printExecutionStats() {
  out << "  Execution stats: ";
  if (execution_status == PRESCHEDULING) {
    out << "Not available before scheduling.\n";
    return;
  }
  unsigned start_cycle = node->get_start_execution_cycle();
  unsigned end_cycle = node->get_complete_execution_cycle();
  unsigned current_cycle = acc->getCurrentCycle();
  if (execution_status == SCHEDULING) {
    if (start_cycle < current_cycle) {
      out << "Scheduling has not reached this node yet.\n";
    } else if (start_cycle >= current_cycle && end_cycle < start_cycle) {
      out << "Started " << start_cycle << ", not yet completed.\n";
    } else if (start_cycle >= current_cycle && end_cycle >= start_cycle) {
      out << "Started " << start_cycle << ", completed " << end_cycle << ".\n";
    }
  } else if (execution_status == POSTSCHEDULING) {
    out << "Started " << start_cycle << ", completed " << end_cycle << ".\n";
  }
}

//-------------------
// DebugEdgePrinter
//-------------------

void DebugEdgePrinter::printAll() {
  printSourceInfo();
  printTargetInfo();
  printEdgeInfo();
}

void DebugEdgePrinter::printSourceInfo() {
  out << "  Source node " << source_node->get_node_id() << ": "
      << source_node->get_microop_name() << "\n";
}

void DebugEdgePrinter::printTargetInfo() {
  out << "  Target node " << target_node->get_node_id() << ": "
      << target_node->get_microop_name() << "\n";
}

void DebugEdgePrinter::printEdgeInfo() {
  if (acc->doesEdgeExist(source_node, target_node)) {
    Edge e = boost::edge(source_node->get_vertex(),
                         target_node->get_vertex(),
                         acc->getGraph())
                 .first;
    int edge_weight = get(boost::edge_name, acc->getGraph())[e];
    out << "  Edge type: ";
    switch (edge_weight) {
      case (uint8_t)MEMORY_EDGE:
        out << "Memory dependence (original)";
        break;
      case 1:
        out << "Memory dependence (load)";
        break;
      case 2:
        out << "Memory dependence (store)";
        break;
      case CONTROL_EDGE:
        out << "Control dependence";
        break;
      default:
        out << edge_weight;
        break;
    }
    out << "\n";

  } else {
    out << "  No edge between these two nodes found!\n";
  }
}

//-------------------
// DebugLoopPrinter
//-------------------

DebugLoopPrinter::LoopIdentifyStatus DebugLoopPrinter::identifyLoop(
    const std::string& loop_name) {
  using namespace SrcTypes;

  Label* label = srcManager.get<Label>(loop_name);
  std::vector<UniqueLabel> candidates;

  const std::multimap<unsigned, UniqueLabel>& labelmap = acc->getLabelMap();
  for (auto it = labelmap.begin(); it != labelmap.end(); ++it) {
    const UniqueLabel& unique_label = it->second;
    if (unique_label.get_label() == label)
      candidates.push_back(unique_label);
  }

  if (candidates.size() == 0) {
    selected_label = UniqueLabel();
    return LOOP_NOT_FOUND;
  } else if (candidates.size() == 1) {
    selected_label = candidates[0];
    return LOOP_FOUND;
  } else {
    // Ask the user to choose between the possibilities.
    out << "Several functions contain labeled statements matching \""
        << loop_name << "\".\n"
        << "Please select one by entering the number, or press <Enter> to "
           "abort.\n";
    for (unsigned i = 0; i < candidates.size(); i++) {
      const UniqueLabel& candidate_label = candidates[i];
      const SrcTypes::Function* func = candidate_label.get_function();
      out << "  " << i + 1 << ": " << func->get_name() << "\n";
    }
    out << "\n";
    int selection = getUserSelection(candidates.size());
    if (selection == -1) {
      return ABORTED;
    } else {
      selected_label = candidates[selection-1];
      return LOOP_FOUND;
    }
  }
}

int DebugLoopPrinter::getUserSelection(int max_option) {
  const char* invalid_selection_msg =
      "Invalid selection! Try again (or <Enter> to quit)\n";

  while (true) {
#ifdef HAS_READLINE
    char* cmd = readline("Selection: ");
    std::string command(cmd);
    free(cmd);
#else
    std::string command;
    std::cout << "Selection: ";
    std::getline(std::cin, command);
#endif
    boost::trim(command);

    if (command.empty())
      return -1;

    int selection = 0;
    try {
      selection = std::stoi(command, NULL, 10);
    } catch (const std::invalid_argument& e) {
      std::cerr << invalid_selection_msg;
      continue;
    }
    if (selection < 1 || selection > max_option) {
      std::cerr << invalid_selection_msg;
      continue;
    }

    return selection;
  }
}

// TODO: New strategy: rather than go branch to branch, go
// branch->next-non-isolated-node to branch.
int DebugLoopPrinter::computeLoopLatency(
    const std::list<node_pair_t>& loop_bound_nodes) {
  int max_latency = 0;
  for (auto it = loop_bound_nodes.begin(); it != loop_bound_nodes.end(); ++it) {
    ExecNode* first = it->first;
    ExecNode* second = it->second;
    int latency = second->get_complete_execution_cycle() -
                  first->get_complete_execution_cycle();
    if (latency > max_latency)
      max_latency = latency;
  }
  return max_latency;
}

void DebugLoopPrinter::printLoop(const std::string &loop_name) {
  LoopIdentifyStatus status = identifyLoop(loop_name);
  if (status == ABORTED) {
    return;
  } else if (status == LOOP_NOT_FOUND) {
    std::cerr << "ERROR: No loops matching the name \"" << loop_name
              << "\" were found!\n";
    return;
  }
  SrcTypes::Function* func = selected_label.get_function();
  out << "Loop \"" << loop_name << "\"\n"
      << "  Function: " << func->get_name() << "\n"
      << "  Line number: " << selected_label.get_line_number() << "\n";

  std::list<node_pair_t> loop_bound_nodes =
      acc->findLoopBoundaries(selected_label);
  out << "  Loop boundaries: ";
  if (loop_bound_nodes.empty()) {
    out << "None.\n";
  } else {
    const unsigned kMaxPrintsPerRow = 4;
    const unsigned kMaxPrints = kMaxPrintsPerRow * 20;
    unsigned num_prints = 0;
    auto bound_it = loop_bound_nodes.begin();
    while (bound_it != loop_bound_nodes.end() &&
           num_prints < kMaxPrints) {
      for (unsigned i = 0; i < kMaxPrintsPerRow; i++) {
        if (bound_it != loop_bound_nodes.end()) {
          out << "[" << bound_it->first->get_node_id() << ", "
              << bound_it->second->get_node_id() << "]  ";
          ++bound_it;
          num_prints++;
        } else {
          break;
        }
      }
      out << "\n";
      if (bound_it != loop_bound_nodes.end())
        out << "                   ";
    }
    if (kMaxPrints < loop_bound_nodes.size())
      out << "... (and " << loop_bound_nodes.size() - kMaxPrints << " more).\n";
  }

  out << "  Latency: ";
  if (execution_status == PRESCHEDULING) {
    out << "Not available before scheduling.\n";
  } else {
    int latency = computeLoopLatency(loop_bound_nodes);
    out << latency << " cycles";
    if (execution_status == SCHEDULING) {
      out << " (may change as scheduling continues).\n";
    } else {
      out << ".\n";
    }
  }
}
