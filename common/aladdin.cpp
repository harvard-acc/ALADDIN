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

  std::cout << logo << std::endl;

  if (argc < 4) {
    std::cout << "-------------------------------" << std::endl;
    std::cout << "Aladdin takes:                 " << std::endl;
    std::cout
        << "./aladdin <bench> <dynamic trace> <config file> <experiment_name>"
        << std::endl;
    std::cout << "   experiment_name is an optional parameter, only used to \n"
              << "   identify results stored in a local database." << std::endl;
    std::cout << "   Aladdin supports gzipped dynamic trace files - append \n"
              << "   the \".gz\" extension to the end of the trace file."
              << std::endl;
    std::cout << "-------------------------------" << std::endl;
    exit(0);
  }
  std::cout << "-------------------------------" << std::endl;
  std::cout << "      Starts Aladdin           " << std::endl;
  std::cout << "-------------------------------" << std::endl;

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

  // Build the graph.
  bool dddg_built = acc->buildDddg();

  // Repeat for each invocation of the accelerator.
  while (dddg_built) {
    acc->globalOptimizationPass();
    /* Profiling */
    acc->prepareForScheduling();

    // Scheduling
    while (!acc->step()) {
    }

    acc->dumpStats();
    acc->clearDatapath();

    dddg_built = acc->buildDddg();
  };

  delete acc;
  return 0;
}
