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

#include <exception>
#include <iostream>
#include <stdio.h>

#include <boost/graph/breadth_first_search.hpp>
#include <boost/tokenizer.hpp>

#include "DDDG.h"
#include "debugger_print.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

#ifdef HAS_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

enum DebugReturnCode {
  CONTINUE,
  QUIT,
  ERR
};

class bfs_finished : public std::exception {
  virtual const char* what() const throw() {
    return "BFS finished";
  }
};

class NodeVisitor : public boost::default_bfs_visitor {
 public:
  NodeVisitor(Graph* _subgraph,
              std::map<unsigned, Vertex>* _existing_nodes,
              ScratchpadDatapath* _acc,
              Vertex _root_vertex,
              unsigned maxnodes,
              unsigned* _num_nodes_visited)
      : new_graph(_subgraph), existing_nodes(_existing_nodes), acc(_acc),
        root_vertex(_root_vertex), max_nodes(maxnodes),
        num_nodes_visited(_num_nodes_visited) {}

  // Insert a vertex with node_id if we aren't above the max_nodes limit.
  Vertex insert_vertex(unsigned node_id) const {
    if (*num_nodes_visited >= max_nodes)
      throw bfs_finished();

    return add_vertex(VertexProperty(node_id), *new_graph);
  }

  // Add a vertex and all of its children to the graph.
  void discover_vertex(const Vertex& v, const Graph& g) const {
    unsigned node_id = get(boost::vertex_node_id, g, v);
    auto node_it = existing_nodes->find(node_id);
    Vertex new_vertex;
    if (node_it == existing_nodes->end()) {
      new_vertex = insert_vertex(node_id);
      existing_nodes->operator[](node_id) = new_vertex;
      (*num_nodes_visited)++;
    } else {
      new_vertex = existing_nodes->at(node_id);
    }

    // If this node is a branch/call, don't print its children (unless this
    // node is the root of the BFS). These nodes tend to have a lot of children.
    if (new_vertex != root_vertex) {
      ExecNode* node = acc->getNodeFromNodeId(node_id);
      if (node->is_branch_op() || node->is_call_op())
        return;
    }

    // Check if any of its parents is a branch/call whose children we ignored.
    // If so, add an edge for each parent we already saw.
    in_edge_iter in_edge_it, in_edge_end;
    for (boost::tie(in_edge_it, in_edge_end) = in_edges(v, g);
         in_edge_it != in_edge_end;
         ++in_edge_it) {
      Vertex source_vertex_orig = source(*in_edge_it, g);
      unsigned source_id = get(boost::vertex_node_id, g, source_vertex_orig);
      auto source_it = existing_nodes->find(source_id);
      if (source_it != existing_nodes->end()) {
        Vertex source_vertex_new = source_it->second;
        unsigned edge_weight = get(boost::edge_name, g, *in_edge_it);
        add_edge(source_vertex_new,
                 new_vertex,
                 EdgeProperty(edge_weight),
                 *new_graph);
      }
    }

    // Immediately add all of its outgoing edges to the graph.
    out_edge_iter out_edge_it, out_edge_end;
    for (boost::tie(out_edge_it, out_edge_end) = out_edges(v, g);
         out_edge_it != out_edge_end;
         ++out_edge_it) {
      Vertex target_vertex_orig = target(*out_edge_it, g);
      unsigned target_id = get(boost::vertex_node_id, g, target_vertex_orig);
      if (existing_nodes->find(target_id) == existing_nodes->end()) {
        Vertex target_vertex = insert_vertex(target_id);
        unsigned edge_weight = get(boost::edge_name, g, *out_edge_it);
        add_edge(
            new_vertex, target_vertex, EdgeProperty(edge_weight), *new_graph);
        existing_nodes->operator[](target_id) = target_vertex;
        (*num_nodes_visited)++;
      }
    }
  }

 private:
  std::map<unsigned, Vertex> *existing_nodes;
  Graph* new_graph;
  ScratchpadDatapath* acc;
  Vertex root_vertex;
  unsigned max_nodes;
  unsigned* num_nodes_visited;
};

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
