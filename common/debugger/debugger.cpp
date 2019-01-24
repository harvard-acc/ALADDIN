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

#include <iostream>
#include <string>

#include "../ScratchpadDatapath.h"

#include "debugger_prompt.h"

using namespace adb;

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

  init_debugger();

  acc = new ScratchpadDatapath(bench, trace_file, config_file);

  // Build the graph.
  acc->buildDddg();

  // Begin interactive mode.
  HandlerRet ret = interactive_mode(acc);
  if (ret == QUIT)
    goto exit;

  acc->globalOptimizationPass();
  acc->prepareForScheduling();

  ret = interactive_mode(acc);
  if (ret == QUIT)
    goto exit;

  // Scheduling
  execution_status = SCHEDULING;
  while (!acc->step()) {}
  acc->dumpStats();

  // Begin interactive mode again.
  execution_status = POSTSCHEDULING;
  ret = interactive_mode(acc);

  acc->clearDatapath();

exit:
  delete acc;
  return 0;
}
