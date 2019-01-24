// Implementation of all debugging command handlers.

#include <iostream>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>

#include <boost/graph/breadth_first_search.hpp>
#include <boost/tokenizer.hpp>

#include "../typedefs.h"
#include "debugger_commands.h"
#include "debugger_graph.h"
#include "debugger_print.h"
#include "debugger_prompt.h"

using namespace adb;

void dump_graph(Graph& graph, ScratchpadDatapath* acc, std::string graph_name) {
  std::unordered_map<Vertex, const ExecNode*> vertexToNode;
  BGL_FORALL_VERTICES(v, graph, Graph) {
    const ExecNode* node =
        acc->getProgram().nodes.at(get(boost::vertex_node_id, graph, v));
    vertexToNode[v] = node;
  }
  std::ofstream out(graph_name + "_graph.dot", std::ofstream::out);
  write_graphviz(out, graph, make_microop_label_writer(vertexToNode, graph));
}

void reconstruct_graph(Graph* new_graph,
                       ScratchpadDatapath* acc,
                       unsigned root_node_id,
                       unsigned num_nodes,
                       int max_node_id,
                       bool show_branch_children) {
  const Graph &g = acc->getProgram().graph;
  const ExecNode* root_node = acc->getProgram().nodes.at(root_node_id);
  Vertex root_vertex = root_node->get_vertex();
  unsigned num_nodes_visited = 0;
  std::map<unsigned, Vertex> existing_nodes;
  NodeVisitor visitor(new_graph,
                      &existing_nodes,
                      acc,
                      root_vertex,
                      num_nodes,
                      max_node_id,
                      show_branch_children,
                      &num_nodes_visited);
  boost::breadth_first_search(g, root_vertex, boost::visitor(visitor));
}

HandlerRet adb::cmd_print_cycle(const CommandTokens& command_tokens,
                                Command* subcmd_list,
                                ScratchpadDatapath* acc) {

  if (command_tokens.size() < 2) {
    std::cerr << "ERROR: Need to specify a cycle to print activity for!\n";
    return HANDLER_ERROR;
  }
  int cycle = -1;
  int max_nodes = 300;  // Default.
  try {
    cycle = std::stoi(command_tokens[1], NULL, 10);
  } catch (const std::invalid_argument& e) {
    std::cerr << "ERROR: Invalid cycle! Must be a nonnegative integer.\n";
    return HANDLER_ERROR;
  }

  CommandTokens args_tokens(++command_tokens.begin(), command_tokens.end());
  CommandArgs args;
  if (parse_command_args(args_tokens, args) != 0)
    return HANDLER_ERROR;
  if (args.find("max_nodes") != args.end())
    max_nodes = args["max_nodes"];

  if (max_nodes <= 0) {
    std::cerr << "ERROR: Cannot specify max_nodes to be <= 0!\n";
    return HANDLER_ERROR;
  }

  DebugCyclePrinter printer(cycle, (unsigned)max_nodes, acc, std::cout);
  printer.printAll();
  return HANDLER_SUCCESS;
}

HandlerRet adb::cmd_print_function(const CommandTokens& command_tokens,
                                   Command* subcmd_list,
                                   ScratchpadDatapath* acc) {
  if (command_tokens.size() < 2) {
    std::cerr << "ERROR: Need to specify a function to print!\n";
    return HANDLER_ERROR;
  }
  std::string function_name = command_tokens[1];
  DebugFunctionPrinter printer(function_name, acc, std::cout);
  printer.printAll();

  return HANDLER_SUCCESS;
}

HandlerRet adb::cmd_print_loop(const CommandTokens& command_tokens,
                               Command* subcmd_list,
                               ScratchpadDatapath* acc) {
  if (command_tokens.size() < 2) {
    std::cerr << "ERROR: Need to specify a loop to print!\n";
    return HANDLER_ERROR;
  }
  std::string loop_name = command_tokens[1];
  DebugLoopPrinter printer(acc, std::cout);
  printer.printLoop(loop_name);

  return HANDLER_SUCCESS;
}

HandlerRet adb::cmd_print_edge(const CommandTokens& command_tokens,
                               Command* subcmd_list,
                               ScratchpadDatapath* acc) {
  if (command_tokens.size() < 3) {
    std::cerr << "ERROR: Need to specify source and target node ids!\n";
    return HANDLER_ERROR;
  }
  unsigned src_node_id = 0xdeadbeef;
  unsigned tgt_node_id = 0xdeadbeef;
  try {
    src_node_id = std::stoi(command_tokens[1], NULL, 10);
    tgt_node_id = std::stoi(command_tokens[2], NULL, 10);
  } catch (const std::invalid_argument& e) {
    std::cerr << "ERROR: Invalid node id! Must be a nonnegative integer.\n";
    return HANDLER_ERROR;
  }

  if (!acc->getProgram().nodeExists(src_node_id)) {
    std::cerr << "ERROR: Source node " << src_node_id << " does not exist!\n";
    return HANDLER_ERROR;
  }
  if (!acc->getProgram().nodeExists(tgt_node_id)) {
    std::cerr << "ERROR: Target node " << tgt_node_id << " does not exist!\n";
    return HANDLER_ERROR;
  }
  const ExecNode* source = acc->getProgram().nodes.at(src_node_id);
  const ExecNode* target = acc->getProgram().nodes.at(tgt_node_id);
  DebugEdgePrinter printer(source, target, acc, std::cout);
  printer.printAll();

  return HANDLER_SUCCESS;
}

HandlerRet adb::cmd_print_node(const CommandTokens& command_tokens,
                               Command* subcmd_list,
                               ScratchpadDatapath* acc) {
  if (command_tokens.size() < 2) {
    std::cerr << "ERROR: Need to specify a node id!\n";
    return HANDLER_ERROR;
  }
  unsigned node_id = 0xdeadbeef;
  try {
    node_id = std::stoi(command_tokens[1], NULL, 10);
  } catch (const std::invalid_argument& e) {
    std::cerr << "ERROR: Invalid node id! Must be a nonnegative integer.\n";
    return HANDLER_ERROR;
  }

  if (!acc->getProgram().nodeExists(node_id)) {
    std::cerr << "ERROR: Node " << node_id << " does not exist!\n";
    return HANDLER_ERROR;
  }
  const ExecNode* node = acc->getProgram().nodes.at(node_id);
  DebugNodePrinter printer(node, acc, std::cout);
  printer.printAll();

  return HANDLER_SUCCESS;
}

HandlerRet adb::cmd_print(const CommandTokens& command_tokens,
                          Command* subcmd_list,
                          ScratchpadDatapath* acc) {
  if (command_tokens.size() < 2) {
    std::cerr << "ERROR: Invalid arguments to 'print'.\n";
    return HANDLER_ERROR;
  }

  CommandTokens subcommand_tokens(++command_tokens.begin(), command_tokens.end());
  HandlerRet ret = dispatch_command(subcommand_tokens, subcmd_list, acc);
  if (ret == HANDLER_NOT_FOUND) {
    std::cerr << "ERROR: Unsupported object to print!\n";
    return HANDLER_ERROR;
  }

  return ret;
}

// graph root=N [max_nodes=K] [show_branch_children=1/0]
HandlerRet adb::cmd_graph(const CommandTokens& command_tokens,
                          Command* subcmd_list,
                          ScratchpadDatapath* acc) {
  if (command_tokens.size() < 2) {
    std::cout << "Invalid arguments to command 'graph'!\n";
    return HANDLER_ERROR;
  }

  int root_node = -1;
  bool show_branch_children = true;  // Default
  int num_nodes = 300;  // Default.
  int max_node_id = -1;  // Default.

  CommandTokens args_tokens(++command_tokens.begin(), command_tokens.end());
  CommandArgs args;
  if (parse_command_args(args_tokens, args) != 0)
    return HANDLER_ERROR;

  if (args.find("root") != args.end()) {
    root_node = args["root"];
  } else {
    std::cerr << "ERROR: Must specify the root node!\n";
    return HANDLER_ERROR;
  }

  if (args.find("num_nodes") != args.end())
    num_nodes = args["num_nodes"];
  if (args.find("max_node_id") != args.end())
    max_node_id = args["max_node_id"];
  if (args.find("show_branch_children") != args.end())
    show_branch_children = args["show_branch_children"];

  if (!acc->getProgram().nodeExists(root_node)) {
    std::cerr << "ERROR: Node " << root_node << " does not exist!\n";
    return HANDLER_ERROR;
  }
  Graph subgraph;
  try {
    reconstruct_graph(&subgraph,
                      acc,
                      root_node,
                      num_nodes,
                      max_node_id,
                      show_branch_children);
  } catch (const bfs_finished &e) {
    // Nothing to do.
  }
  dump_graph(subgraph, acc, "debug");
  std::cout << "Graph has been written to debug_graph.dot.\n";

  return HANDLER_SUCCESS;
}

HandlerRet adb::cmd_help(const CommandTokens& tokens,
                         Command* subcmd_list,
                         ScratchpadDatapath* acc) {
  std::cout << "\nAladdin debugger help\n"
            << "========================\n\n"
            << "The Aladdin debugger works just like Aladdin itself, except after Aladdin runs the\n"
            << "global optimization pass, it will pause and allow the user to query information\n"
            << "about the DDDG. The user can print details about individual nodes and see them in\n"
            << "a format that is more readable than the raw trace. Additionally, the user can\n"
            << "request a dump of a portion of the DDDG to visually inspect the dependencies\n"
            << "between nodes, as the entire DDDG is generally too large for any graph drawing\n"
            << "program to render.\n\n"
            << "The debugger will interrupt Aladdin's execution at two places: after the global\n"
            << "optimization pass, and after all the scheduling has taken place.\n\n"
            << "Note: the debugger cannot alter any Aladdin state. It can only report the state\n"
            << "in its current form.\n\n"
            << "Supported commands:\n"
            << "  help                            : Print this message\n"
            << "\n"
            << "  print node [id]                 : Print details about this node\n"
            << "  print loop [label-name]         : Print details about the loop labeled by label-name.\n"
            << "         If multiple functions contain such a label, the user is prompted to select\n"
            << "         the correct one. Details include the average latency of the loop and a list of\n"
            << "         the branch nodes that correspond to this loop header. Technically, this will\n"
            << "         work for any labeled statement, but the loop statistics would not be present.\n"
            << "  print function [function-name]  : Print details about this function\n"
            << "  print edge [node-0] [node-1]    : Print details about edges between the two nodes.\n"
            << "  print cycle [cycle]             : Print which nodes were executing at this cycle.\n"
            << "    Optional arguments:\n"
            << "      max_nodes=M                 : Print up to M nodes. Default: 300.\n"
            << "\n"
            << "  graph root=[node-id]            : Dump the DDDG in BFS fashion, with node-id as the root.\n"
            << "    Optional arguments:\n"
            << "      num_nodes=M              : Graph up to M nodes. Default: 300.\n"
            << "      max_node_id=N            : Don't show any nodes greater than this ID. Default: unlimited.\n"
            << "      show_branch_children=1/0 : Include edges to the children of all branch and call nodes.\n"
            << "           By default, include (1). Set to 0 to exclude.\n"
            << "           Branch and call nodes tend to have a lot of child dependent nodes that may\n"
            << "           not be dependent on each other (e.g. different iterations of the same or\n"
            << "           different loop), so you can exclude them to keep the output cleaner.\n"
            << "\n"
            << "  continue                           : Continue executing Aladdin\n"
            << "  quit                               : Quit the debugger.\n";

  return HANDLER_SUCCESS;
}

HandlerRet adb::cmd_continue(const CommandTokens& tokens,
                             Command* subcmd_list,
                             ScratchpadDatapath* acc) {
  return CONTINUE;
}

HandlerRet adb::cmd_quit(const CommandTokens& tokens,
                         Command* subcmd_list,
                         ScratchpadDatapath* acc) {
  return QUIT;
}
