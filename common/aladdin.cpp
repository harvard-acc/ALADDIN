#include "file_func.h"
#include "ScratchpadDatapath.h"
#include "Scratchpad.h"
#include "DDDG.h"
#include <stdio.h>

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

  std::cout << logo << endl;

  if (argc < 4) {
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << "Aladdin takes:                 " << std::endl;
    std::cerr
        << "./aladdin <bench> <dynamic trace> <config file> <experiment_name>"
        << endl;
    std::cerr << "   experiment_name is an optional parameter, only used to \n"
              << "   identify results stored in a local database." << std::endl;
    std::cerr << "   Aladdin supports gzipped dynamic trace files - append \n"
              << "   the \".gz\" extension to the end of the trace file."
              << std::endl;
    std::cerr << "-------------------------------" << std::endl;
    exit(0);
  }
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "      Starts Aladdin           " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  std::string bench(argv[1]);
  std::string trace_file(argv[2]);
  std::string config_file(argv[3]);
  std::string experiment_name;

  std::cout << bench << "," << trace_file << "," << config_file << ","
            << std::endl;

  ScratchpadDatapath* acc;

  acc = new ScratchpadDatapath(bench, trace_file, config_file);
#ifdef USE_DB
  bool use_db = (argc == 5);
  if (use_db) {
    experiment_name = std::string(argv[4]);
    acc->setExperimentParameters(experiment_name);
  }
#endif
  // Get the complete graph.
  acc->setGlobalGraph();
  acc->globalOptimizationPass();
  /* Profiling */
  acc->setGraphForStepping();

  // Scheduling
  while (!acc->step()) {
  }
  int cycles = acc->clearGraph();

  acc->dumpStats();
  delete acc;
  return 0;
}
