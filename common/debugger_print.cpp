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
  SrcTypes::Function func =
      srcManager.get<SrcTypes::Function>(node->get_static_function_id());
  if (func != InvalidFunction) {
    out << "  Parent function: " << func.get_name() << "\n";
    out << "  Dynamic invocation count: " << node->get_dynamic_invocation()
        << "\n";
  }

  if (line_num != -1 && func != InvalidFunction) {
    const std::multimap<unsigned, UniqueLabel>& labelmap =
        acc->getLabelMap();
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

    SrcTypes::Function curr_func =
        srcManager.get<SrcTypes::Function>(node->get_static_function_id());
    out << "  Called function: ";
    if (called_node) {
      SrcTypes::Function called_func = srcManager.get<SrcTypes::Function>(
          called_node->get_static_function_id());
      if (called_func != curr_func && called_func != InvalidFunction)
        out << called_func.get_name() << "\n";
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
// DebugLoopPrinter
//-------------------

DebugLoopPrinter::LoopIdentifyStatus DebugLoopPrinter::identifyLoop(
    const std::string& loop_name) {
  using namespace SrcTypes;

  src_id_t label_id = srcManager.get_id<Label>(loop_name);
  if (label_id == InvalidId)
    return LOOP_NOT_FOUND;

  Label label = srcManager.get<Label>(label_id);
  std::vector<std::pair<UniqueLabel, unsigned>> candidates;

  const std::multimap<unsigned, UniqueLabel>& labelmap = acc->getLabelMap();
  for (auto it = labelmap.begin(); it != labelmap.end(); ++it) {
    const UniqueLabel& unique_label = it->second;
    if (unique_label.get_label_id() == label.get_id())
      candidates.push_back(std::make_pair(unique_label, it->first));
  }

  if (candidates.size() == 0) {
    selected_label = std::make_pair(UniqueLabel(InvalidId, InvalidId), 0);
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
      UniqueLabel& candidate_label = candidates[i].first;
      const SrcTypes::Function& func =
          srcManager.get<SrcTypes::Function>(candidate_label.get_function_id());
      out << "  " << i + 1 << ": " << func.get_name() << "\n";
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

std::list<DebugLoopPrinter::node_pair_t> DebugLoopPrinter::findLoopBoundaries() {
  using namespace SrcTypes;
  const UniqueLabel& label = selected_label.first;
  const unsigned line_num = selected_label.second;
  const std::vector<DynLoopBound>& all_loop_bounds = acc->getLoopBoundaries();
  std::list<node_pair_t> loop_boundaries;
  bool is_loop_executing = false;
  int current_loop_depth = -1;

  ExecNode* loop_start;

  // The loop boundaries provided by the accelerator are in a linear list with
  // no structure. We need to identify the start and end of each unrolled loop
  // section and add the corresponding pair of nodes to loop_boundaries.
  //
  // The last loop bound node is one past the last node, so we'll ignore it.
  for (auto it = all_loop_bounds.begin(); it != --all_loop_bounds.end(); ++it) {
    const DynLoopBound& loop_bound = *it;
    ExecNode* node = acc->getNodeFromNodeId(loop_bound.node_id);
    if (!node->is_isolated() &&
        node->get_line_num() == line_num &&
        node->get_static_function_id() == label.get_function_id()) {
      if (!is_loop_executing) {
        is_loop_executing = true;
        loop_start = node;
        current_loop_depth = loop_bound.target_loop_depth;
      } else {
        if (loop_bound.target_loop_depth == current_loop_depth) {
          // We're repeating an iteration of the same loop body.
          loop_boundaries.push_back(std::make_pair(loop_start, node));
          loop_start = node;
        } else if (loop_bound.target_loop_depth < current_loop_depth) {
          // We've left the loop.
          loop_boundaries.push_back(std::make_pair(loop_start, node));
          is_loop_executing = false;
          loop_start = NULL;
        } else if (loop_bound.target_loop_depth > current_loop_depth) {
          // We've entered an inner loop nest. We're not interested in the
          // inner loop nest.
          continue;
        }
      }
    }
  }
  return loop_boundaries;
}

// TODO: New strategy: rather than go branch to branch, go
// branch->next-non-isolated-node to branch.
int DebugLoopPrinter::computeLoopLatency(
    const std::list<DebugLoopPrinter::node_pair_t>& loop_bound_nodes) {
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
  const SrcTypes::Function& func =
      srcManager.get<SrcTypes::Function>(selected_label.first.get_function_id());
  out << "Loop \"" << loop_name << "\"\n"
      << "  Function: " << func.get_name() << "\n"
      << "  Line number: " << selected_label.second << "\n";

  std::list<node_pair_t> loop_bound_nodes = findLoopBoundaries();
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
