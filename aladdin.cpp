#include "file_func.h"
#include "Datapath.h"
#include "Scratchpad.h"
#include "dddg.h"
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

  string bench(argv[1]);
  string trace_file(argv[2]);
  string config_file(argv[3]);
  
  cout << bench << "," << trace_file << "," << config_file <<  endl;
  /*Build Initial DDDG*/
  if (!build_initial_dddg(bench, trace_file))
  {
    std::cerr << "-------------------------------" << std::endl;
    std::cerr << "       Aladdin Ends..          " << std::endl;
    std::cerr << "-------------------------------" << std::endl;
    exit(0);
  }
  
  parse_config(bench, config_file);
  
  
  Datapath *acc;
  Scratchpad *spad;

  spad = new Scratchpad(1); 
  acc = new Datapath(bench, CYCLE_TIME);
  acc->setScratchpad(spad);
  //get the complete graph
  acc->setGlobalGraph();
  acc->globalOptimizationPass();
  
  /*Profiling*/
  acc->clearGlobalGraph();
  vector<string> v_method_order;
  unordered_map<string, int > map_method_2_callinst;
  init_method_order(bench, v_method_order, map_method_2_callinst);
  
  ofstream method_latency;
  method_latency.open(bench+ "_method_latency", ofstream::out);
  for(unsigned method_id = 0; method_id < v_method_order.size(); ++method_id)
  {
    string current_dynamic_method, graph_file;
    int min_node = 0;
    if (method_id != v_method_order.size() -1)
    {
      current_dynamic_method = v_method_order.at(method_id);
      graph_file = bench + "_" + current_dynamic_method;
      min_node = map_method_2_callinst[current_dynamic_method] + 1;
    }
    else
    {
      current_dynamic_method = bench;
      graph_file = bench;
      min_node = 0;
    }
    
    //Per Dynamic Function Local Optimization
    acc->setGraphName(graph_file, min_node);
    acc->optimizationPass();
    acc->setGraphForStepping(graph_file);
    
    //Scheduling
    while(!acc->step())
      spad->step();
    int cycles = acc->clearGraph();
    method_latency  << current_dynamic_method  << "," << cycles << endl;
  }
  method_latency.close();
  acc->dumpStats();
  delete acc;
  delete spad;
  return 0;
}
