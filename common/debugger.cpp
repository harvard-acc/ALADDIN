// A debugging interface for Aladdin.
//
// The debugger makes it much simpler to inspect the DDDG and find out why
// Aladdin is not providing expected performance/power/area estimates. It can
// print information about individual nodes, including all of its parents and
// children (which cannot be easily deduced by looking at the trace alone). It
// can also dump a subgraph of the DDDG in Graphviz format, making visual
// inspection possible (as the entire DDDG is too large to visualize).
//
// This is a drop in replacement for Aladdin, so run it just like standalone
// Aladdin but with the different executable name.
//
// For best results, install libreadline (any recent version will do) to enable
// features like accessing command history (C-r to search, up/down arrows to go
// back and forth).
//
// For more information, see the help command.

#include <cstdlib>
#include <exception>
#include <iostream>
#include <signal.h>
#include <stdio.h>

#include <boost/graph/breadth_first_search.hpp>
#include <boost/tokenizer.hpp>

#include "DDDG.h"
#include "debugger_graph.h"
#include "debugger_print.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

#ifdef HAS_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

// Determines how SIGINT is handled. If we're waiting for an input, then we
// want to resume the application; otherwise, we want to kill it.
static sig_atomic_t waiting_for_input;

enum DebugReturnCode {
  CONTINUE,
  QUIT,
  ERR
};

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

DebugReturnCode cmd_print(std::vector<std::string>& command_tokens,
                         ScratchpadDatapath* acc) {

  unsigned num_tokens = command_tokens.size();
  if (num_tokens < 2) {
    std::cerr << "ERROR: Invalid arguments to 'print'.\n";
    return ERR;
  }
  std::string print_type = command_tokens[1];
  if (print_type == "node") {
    if (num_tokens == 2) {
      std::cerr << "ERROR: Need to specify a node id!\n";
      return ERR;
    }
    unsigned node_id = 0xdeadbeef;
    try {
      node_id = std::stoi(command_tokens[2], NULL, 10);
    } catch (const std::invalid_argument &e) {
      std::cerr << "ERROR: Invalid node id! Must be a nonnegative integer.\n";
      return ERR;
    }

    if (!acc->doesNodeExist(node_id)) {
      std::cerr << "ERROR: Node " << node_id << " does not exist!\n";
      return ERR;
    }
    ExecNode* node = acc->getNodeFromNodeId(node_id);
    DebugNodePrinter printer(node, acc, std::cout);
    printer.printAll();
  } else if (print_type == "loop") {
    if (num_tokens < 3) {
      std::cerr << "ERROR: Need to specify a loop to print!\n";
      return ERR;
    }
    std::string loop_name = command_tokens[2];
    DebugLoopPrinter printer(acc, std::cout);
    printer.printLoop(loop_name);
  } else {
    std::cerr << "ERROR: Unsupported object to print!\n";
    return ERR;
  }
  return CONTINUE;
}

// graph root=N [maxnodes=K]
DebugReturnCode cmd_graph(std::vector<std::string>& command_tokens,
                          ScratchpadDatapath* acc) {
  if (command_tokens.size() < 2) {
    std::cout << "Invalid arguments to command 'graph'!\n";
    return ERR;
  }

  int root_node = -1;
  int maxnodes = -1;

  boost::char_separator<char> sep("=");
  for (unsigned i = 1; i < command_tokens.size(); i++) {
    boost::tokenizer<boost::char_separator<char>> tok(command_tokens[i], sep);
    std::string arg;
    int value = -1;
    int argnum = 0;
    for (auto it = tok.begin(); it != tok.end(); ++it, ++argnum) {
      if (argnum == 0) {
        arg = *it;
        continue;
      } else if (argnum == 1) {
        try {
          value = stoi(*it);
        } catch (const std::invalid_argument &e) {
          std::cerr << "ERROR: Invalid argument " << *it << " to parameter " << arg << ".\n";
          return ERR;
        }
      }
      if (value == -1) {
        std::cerr << "ERROR: Missing value to parameter " << arg << ".\n";
        return ERR;
      }
      if (arg == "root") {
        root_node = value;
      } else if (arg == "maxnodes") {
        maxnodes = value;
      } else {
        std::cerr << "ERROR: Unknown parameter " << arg << " to command 'graph'.\n";
        return ERR;
      }
    }
  }

  if (root_node == -1) {
    std::cerr << "ERROR: Must specify the root node!\n";
    return ERR;
  }

  // Set default values for missing parameters.
  if (maxnodes == -1)
    maxnodes = 100;

  if (!acc->doesNodeExist(root_node)) {
    std::cerr << "ERROR: Node " << root_node << " does not exist!\n";
    return ERR;
  }
  Graph subgraph;
  try {
    reconstruct_graph(&subgraph, acc, root_node, maxnodes);
  } catch (const bfs_finished &e) {
  }
  dump_graph(subgraph, acc, "debug");
  std::cout << "Graph has been written to debug_graph.dot.\n";
  return CONTINUE;
}

void cmd_help() {
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
}

void cmd_unknown(std::string& command) {
  std::cout << "\nUnknown command " << command << std::endl;
}

std::string get_command() {
  std::string command;
  waiting_for_input = 1;
#ifdef HAS_READLINE
  char* cmd = readline("aladdin >> ");
  if (cmd && *cmd)
    add_history(cmd);
  command = std::string(cmd);
  free(cmd);
#else
  std::cout << "aladdin >> ";
  std::getline(std::cin, command);
#endif
  waiting_for_input = 0;
  return command;
}
// Supported commands.
//
// help
// print node <id>
// print array <array name>  [ PLANNED ]
// graph <root node> <maxnodes>
// continue
// quit
DebugReturnCode interactive_mode(ScratchpadDatapath* acc) {
  std::cout << "Entering Aladdin Debugger...\n";
  while (true) {
    std::string command = get_command();

    if (command.empty()) {
      continue;
    }

    boost::char_separator<char> sep(" ");
    boost::tokenizer<boost::char_separator<char>> tok(command, sep);
    std::vector<std::string> command_split;
    unsigned arg_num = 0;
    for (auto it = tok.begin(); it != tok.end(); ++it) {
      command_split.push_back(*it);
    }

    if (command_split.size() == 0) {
      continue;
    }

    // Execute the command.
    std::string root_command = command_split[0];
    if (root_command == "quit") {
      return QUIT;
    } else if (root_command == "continue") {
      return CONTINUE;
    } else if (root_command == "print") {
      cmd_print(command_split, acc);
    } else if (root_command == "graph") {
      cmd_graph(command_split, acc);
    } else if (root_command == "help") {
      cmd_help();
    } else {
      cmd_unknown(command);
    }
  }
}

#ifdef HAS_READLINE
void handle_sigint(int signo) {
  if (waiting_for_input) {
    // Move to a new line and clear the existing one.
    printf("\n");
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
    // Reregister the signal handler.
    signal(SIGINT, handle_sigint);
    return;
  }
  exit(1);
}

void init_readline() {
  // Installs signal handlers to capture SIGINT, so we can move to a new prompt
  // instead of ending the process.
  signal(SIGINT, handle_sigint);
}
#endif

int main(int argc, const char* argv[]) {
  const char* logo =
      "     ________                                                    \n"
      "    /\\ ____  \\    ___   _       ___  ______ ______  _____  _   _ \n"
      "   /  \\    \\  |  / _ \\ | |     / _ \\ |  _  \\|  _  \\|_   _|| \\ | "
      "|\n"
      "  / /\\ \\    | | / /_\\ \\| |    / /_\\ \\| | | || | | |  | |  |  \\| "
      "|\n"
      " | |  | |   | | |  _  || |    |  _  || | | || | | |  | |  | . ` |\n"
      " \\ \\  / /__/  | | | | || |____| | | || |/ / | |/ /  _| |_ | |\\  |\n"
      "  \\_\\/_/ ____/  \\_| |_/\\_____/\\_| |_/|___/  |___/  |_____|\\_| "
      "\\_/\n"
      "                                                                 \n";

  std::cout << logo << std::endl;

  if (argc < 4) {
    std::cout << "-------------------------------" << std::endl;
    std::cout << "Aladdin Debugger Usage:    " << std::endl;
    std::cout
        << "./debugger <bench> <dynamic trace> <config file>"
        << std::endl;
    std::cout << "   Aladdin supports gzipped dynamic trace files - append \n"
              << "   the \".gz\" extension to the end of the trace file."
              << std::endl;
    std::cout << "-------------------------------" << std::endl;
    exit(0);
  }

  std::string bench(argv[1]);
  std::string trace_file(argv[2]);
  std::string config_file(argv[3]);

  std::cout << bench << "," << trace_file << "," << config_file << ","
            << std::endl;

  ScratchpadDatapath* acc;

#ifdef HAS_READLINE
  init_readline();
#endif
  waiting_for_input = 0;

  acc = new ScratchpadDatapath(bench, trace_file, config_file);

  // Build the graph.
  acc->buildDddg();
  acc->globalOptimizationPass();
  acc->prepareForScheduling();

  // Begin interactive mode.
  DebugReturnCode ret = interactive_mode(acc);
  if (ret == QUIT) {
    goto exit;
  }

  // Scheduling
  while (!acc->step()) {}
  acc->dumpStats();

  // Begin interactive mode again.
  ret = interactive_mode(acc);

  acc->clearDatapath();

exit:
  delete acc;
  return 0;
}
