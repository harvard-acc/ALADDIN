#include "file_func.h"
#include "Datapath.h"
#include "Scratchpad.h"
#include "dddg.h"
#include <stdio.h>

int main( int argc, const char *argv[])
{
  if(argc < 4)
  {
    cout << "Aladdin" << endl;
    cout << "./aladdin <bench> <dynamic trace> <config file>" << endl;
    exit(0);
  }

  string bench(argv[1]);
  string trace_file(argv[2]);
  string config_file(argv[3]);
  cout << bench << "," << trace_file << "," << config_file <<  endl;
  parse_config(bench, config_file);
  
  /*Build Initial DDDG*/
  build_initial_dddg(bench, trace_file);
  
  Datapath *acc;
  Scratchpad *spad;

  float mem_latency = 1;
  float cycle_time = 10;
  spad = new Scratchpad(mem_latency, 1); 
  acc = new Datapath(bench, cycle_time);
  acc->setScratchpad(spad);
  //get the complete graph
  acc->setGlobalGraph();
  acc->globalOptimizationPass();
  
  /*Profiling*/
  
  acc->clearGlobalGraph();
  cerr << "after clearing global graph" << endl;
  vector<string> v_method_order;
  unordered_map<string, int > map_method_2_callinst;
  cerr << "before init method order " << endl;
  init_method_order(bench, v_method_order, map_method_2_callinst);
  
  cerr << "start per graph computation" << endl;
  
  ofstream method_latency;
  method_latency.open(bench+ "_method_latency", ofstream::out);
  if (v_method_order.size() == 0)
    v_method_order.push_back(bench);
  cerr << "method size : " << v_method_order.size() << endl;
  for(unsigned method_id = 0; method_id < v_method_order.size(); ++method_id)
  {
    string current_dynamic_method;
    string graph_file;
    int min_node = 0;
    fprintf(stderr, "CURRENT METHOD: %s\n", v_method_order.at(method_id).c_str());
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
    
    fprintf(stderr, "CURRENT METHOD: %s\n", current_dynamic_method.c_str());
    unsigned base_method;
    int dynamic_count;
    char dash;
    istringstream parser(current_dynamic_method);
    parser >> base_method >> dash >> dynamic_count;
    
    cerr << current_dynamic_method << endl;

    acc->setGraphName(graph_file, min_node);
    acc->optimizationPass();
    //graph, edgeLatency, edgeType fixed
    
    cerr << current_dynamic_method  << endl;
    acc->setGraphForStepping(graph_file);
    while(!acc->step())
      spad->step();
    int cycles = acc->clearGraph();
    cerr << current_dynamic_method << "," << cycles << endl;
    method_latency 
      << current_dynamic_method  << ","
      //<< map_method_2_callinst[current_dynamic_method]  << ","
      << cycles << endl;
  }
  method_latency.close();
  acc->dumpStats();
  delete acc;
  delete spad;
  return 0;

}
