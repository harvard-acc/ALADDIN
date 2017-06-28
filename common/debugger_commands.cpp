// Implementation of all debugging command handlers.

#include <iostream>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>

#include <boost/graph/breadth_first_search.hpp>
#include <boost/tokenizer.hpp>

#include "typedefs.h"
#include "debugger.h"
#include "debugger_graph.h"
#include "debugger_print.h"
#include "debugger_commands.h"

void dump_graph(Graph& graph, ScratchpadDatapath* acc, std::string graph_name) {
  std::unordered_map<Vertex, unsigned> vertexToMicroop;
  BGL_FORALL_VERTICES(v, graph, Graph) {
    ExecNode* node = acc->getNodeFromNodeId(get(boost::vertex_node_id, graph, v));
    vertexToMicroop[v] = node->get_microop();
  }
  std::ofstream out(graph_name + "_graph.dot", std::ofstream::out);
  write_graphviz(out, graph, make_microop_label_writer(vertexToMicroop, graph));
}

void reconstruct_graph(Graph* new_graph,
                       ScratchpadDatapath* acc,
                       unsigned root_node_id,
                       unsigned maxnodes) {
  const Graph &g = acc->getGraph();
  ExecNode* root_node = acc->getNodeFromNodeId(root_node_id);
  Vertex root_vertex = root_node->get_vertex();
  unsigned num_nodes_visited = 0;
  std::map<unsigned, Vertex> existing_nodes;
  NodeVisitor visitor(new_graph,
                      &existing_nodes,
                      acc,
                      root_vertex,
                      maxnodes,
                      &num_nodes_visited);
  boost::breadth_first_search(g, root_vertex, boost::visitor(visitor));
}

HandlerRet cmd_print_loop(const CommandTokens& command_tokens,
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

HandlerRet cmd_print_node(const CommandTokens& command_tokens,
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

  if (!acc->doesNodeExist(node_id)) {
    std::cerr << "ERROR: Node " << node_id << " does not exist!\n";
    return HANDLER_ERROR;
  }
  ExecNode* node = acc->getNodeFromNodeId(node_id);
  DebugNodePrinter printer(node, acc, std::cout);
  printer.printAll();

  return HANDLER_SUCCESS;
}

HandlerRet cmd_print(const CommandTokens& command_tokens,
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

// graph root=N [maxnodes=K]
HandlerRet cmd_graph(const CommandTokens& command_tokens,
                     Command* subcmd_list,
                     ScratchpadDatapath* acc) {
  if (command_tokens.size() < 2) {
    std::cout << "Invalid arguments to command 'graph'!\n";
    return HANDLER_ERROR;
  }

  int root_node = -1;
  int maxnodes = 100;  // Default.

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

  if (args.find("maxnodes") != args.end()) {
    maxnodes = args["maxnodes"];
  }

  if (!acc->doesNodeExist(root_node)) {
    std::cerr << "ERROR: Node " << root_node << " does not exist!\n";
    return HANDLER_ERROR;
  }
  Graph subgraph;
  try {
    reconstruct_graph(&subgraph, acc, root_node, maxnodes);
  } catch (const bfs_finished &e) {
    // Nothing to do.
  }
  dump_graph(subgraph, acc, "debug");
  std::cout << "Graph has been written to debug_graph.dot.\n";

  return HANDLER_SUCCESS;
}

HandlerRet cmd_help(const CommandTokens& tokens,
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
            << "  help                               : Print this message\n"
            << "\n"
            << "  print node [id]                    : Print details about this node\n"
            << "  print loop [label-name]            : Print details about the loop labeled by label-name.\n"
            << "         If multiple functions contain such a label, the user is prompted to select\n"
            << "         the correct one. Details include the average latency of the loop and a list of\n"
            << "         the branch nodes that correspond to this loop header. Technically, this will\n"
            << "         work for any labeled statement, but the loop statistics would not be present.\n"
            << "\n"
            << "  graph root=node-id <maxnodes=N>    : Dump the DDDG in BFS fashion, starting from\n"
            << "         the specified root node. maxnodes indicates the maximum number of nodes to\n"
            << "         dump (since large graphs can cause rendering programs to choke. If maxnodes\n"
            << "         is not specified, it defaults to 100.\n\n"
            << "         NOTE: Branch and call nodes tend to have a lot of child dependent nodes\n"
            << "         that are usually not dependent on each other (e.g. different iterations of the\n"
            << "         same or different loop). So, unless it is the specified root node, the children\n"
            << "         of branch and call nodes will not get printed.\n"
            << "\n"
            << "  continue                           : Continue executing Aladdin\n"
            << "  quit                               : Quit the debugger.\n";

  return HANDLER_SUCCESS;
}

HandlerRet cmd_continue(const CommandTokens& tokens,
                        Command* subcmd_list,
                        ScratchpadDatapath* acc) {
  return CONTINUE;
}

HandlerRet cmd_quit(const CommandTokens& tokens,
                    Command* subcmd_list,
                    ScratchpadDatapath* acc) {
  return QUIT;
}
