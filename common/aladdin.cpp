#include "file_func.h"
#include "ScratchpadDatapath.h"
#include "Scratchpad.h"
#include "DDDG.h"
#include <stdio.h>

int main( int argc, const char *argv[])
{
   const char *logo="     ________                                                    \n"
                    "    /\\ ____  \\    ___   _       ___  ______ ______  _____  _   _ \n"
                    "   /  \\    \\  |  / _ \\ | |     / _ \\ |  _  \\|  _  \\|_   _|| \\ | |\n"
                    "  / /\\ \\    | | / /_\\ \\| |    / /_\\ \\| | | || | | |  | |  |  \\| |\n"
                    " | |  | |   | | |  _  || |    |  _  || | | || | | |  | |  | . ` |\n"
                    " \\ \\  / /__/  | | | | || |____| | | || |/ / | |/ /  _| |_ | |\\  |\n"
                    "  \\_\\/_/ ____/  \\_| |_/\\_____/\\_| |_/|___/  |___/  |_____|\\_| \\_/\n"
                    "                                                                 \n";

   std::cout << logo << endl;


  if(argc < 4)
  {
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << "Aladdin takes:                 " << std::endl;
    std::cerr << "./aladdin <bench> <dynamic trace> <config file>" << endl;
    std::cerr << "-------------------------------" << std::endl;
    exit(0);
  }
  std::cerr << "-------------------------------" << std::endl;
  std::cerr << "      Starts Aladdin           " << std::endl;
  std::cerr << "-------------------------------" << std::endl;

  std::string bench(argv[1]);
  std::string trace_file(argv[2]);
  std::string config_file(argv[3]);

  std::cout << bench << "," << trace_file << "," << config_file <<  std::endl;

  ScratchpadDatapath *acc;
  Scratchpad *spad;

  spad = new Scratchpad(1, CYCLE_TIME);
  acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
  acc->setScratchpad(spad);
  //get the complete graph
  acc->setGlobalGraph();
  acc->globalOptimizationPass();
  /*Profiling*/
  acc->setGraphForStepping();

  //Scheduling
  while(!acc->step())
    spad->step();
  int cycles = acc->clearGraph();

  acc->dumpStats();
  delete acc;
  delete spad;
  return 0;
}
