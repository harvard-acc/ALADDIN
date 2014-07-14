#include "./Datapath.h"

Datapath::Datapath(string bench, float cycle_t)
{
  benchName = (char*) bench.c_str();
  cycleTime = cycle_t;
  cerr << "Initializing Datapath " << endl;
  string bn(benchName);
  read_gzip_file_no_size(bn + "_microop.gz", microop);
  numTotalNodes = microop.size();
  //also need to read address
  //baseAddress.assign(numTotalNodes, "");
  cycle = 0;
  cerr << "End Initializing Datapath " << endl;
}
Datapath::~Datapath()
{
}

void Datapath::setScratchpad(Scratchpad *spad)
{
#ifdef DEBUG
  std::cerr << "=======Setting Scratchpad=====" << std::endl;
#endif
  scratchpad = spad;
}

//optimizationFunctions
void Datapath::setGlobalGraph()
{
#ifdef DEBUG
  std::cerr << "=======Setting Global Graph=====" << std::endl;
#endif
  graphName = benchName;
}

void Datapath::clearGlobalGraph()
{
#ifdef DEBUG
  std::cerr << "=======Clearing Global Graph=====" << std::endl;
#endif
  writeMicroop(microop);
  newLevel.assign(numTotalNodes, 0);
  globalIsolated.assign(numTotalNodes, 1);
  
  regStats.assign(numTotalNodes, {0, 0, 0});
}

void Datapath::globalOptimizationPass()
{
#ifdef DEBUG
  std::cerr << "=======Global Optimization Pass=====" << std::endl;
#endif
  //graph independent
  ////remove induction variables
  removeInductionDependence();
  /* 
  ////complete partition
  completePartition();
  //partition
  scratchpadPartition();
  //node strength reduction
  ////remove address calculation
  removeAddressCalculation();
  ////remove branch edges
  removeBranchEdges();
  //have to happen after address calculation:FIXME
  nodeStrengthReduction();
  //loop flattening
  loopFlatten();
  
  addCallDependence();
  methodGraphBuilder();
  methodGraphSplitter();
  */
}
void Datapath::loopFlatten()
{
  std::unordered_set<string> flatten_config;
  if (!readFlattenConfig(flatten_config))
    return;
#ifdef DEBUG
  std::cerr << "=======Flatten Loops=====" << std::endl;
#endif
  std::vector<int> methodid(numTotalNodes, 0);
  initMethodID(methodid);
  
  std::vector<int> instid(numTotalNodes, 0);
  initInstID(instid);
  
  int num_of_conversion= 0;

  for(int node_id = 0; node_id < numTotalNodes; node_id++)
  {
    int node_instid = instid.at(node_id);
    int node_methodid = methodid.at(node_id);
    ostringstream oss;
    oss << node_methodid << "-" << node_instid ;
    auto it = flatten_config.find(oss.str());
    if (it == flatten_config.end())
      continue;
    microop.at(node_id) = IRINDEXSTORE;
    num_of_conversion++;
  }
}
/*
 * Modify: graph, edgetype, edgelatency, baseAddress, microop
 * */
void Datapath::completePartition()
{
#ifdef DEBUG
  std::cerr << "=======Complete Partition=====" << std::endl;
#endif
  int changed_nodes = 0;
  
  string bn(benchName);
  string comp_partition_file;
  comp_partition_file = bn + "_complete_partition_config";
  if (!fileExists(comp_partition_file))
    return;
  
  std::unordered_set<unsigned> comp_part_config;
  readCompletePartitionConfig(comp_part_config);
  
  std::vector<unsigned> orig_base(numTotalNodes, 0);
  initMemBaseInNumber(orig_base);
  
  //set graph
  Graph tmp_graph;
  readGraph(tmp_graph); 

  VertexNameMap vertex_to_name = get(boost::vertex_name, tmp_graph);
  EdgeNameMap edge_to_name = get(boost::edge_name, tmp_graph);
  
  unsigned num_of_edges = boost::num_edges(tmp_graph);
#ifdef DEBUG
  std::cerr << "num_edges," << num_of_edges << std::endl;
#endif
  
  std::vector<int> edge_parid (num_of_edges, 0);
  initEdgeParID(edge_parid);
  //read edgetype.second
  std::vector<bool> to_remove_edges(num_of_edges, 0);
  
  vertex_iter vi, vi_end;
  for (tie(vi, vi_end) = vertices(tmp_graph); vi != vi_end; ++vi)
  {
    int node_id = vertex_to_name[*vi];
    int node_microop = microop.at(node_id);
    if (!is_memory_op(node_microop))
      continue;
    unsigned base_addr = orig_base.at(node_id);
    if (base_addr == 0)
      continue;
    if (comp_part_config.find(base_addr) == comp_part_config.end())
      continue;
    changed_nodes++;
    //iterate its parents
    in_edge_iter in_i, in_end;
    for (tie(in_i, in_end) = in_edges(*vi , tmp_graph); in_i != in_end; ++in_i)
    {
      int edge_id = edge_to_name[*in_i];
      int parid = edge_parid.at(edge_id);
      if (parid ==1 )
        to_remove_edges.at(edge_id) = 1;
    }
    orig_base.at(node_id) = 0;
    microop.at(node_id) = IRSTORE;
  }
  writeMemBaseInNumber(orig_base);
  writeGraphWithIsolatedEdges(to_remove_edges);
#ifdef DEBUG
  std::cerr << "=======ChangedNodes: " << changed_nodes << "=====" << std::endl;
#endif
  cleanLeafNodes();
}
void Datapath::cleanLeafNodes()
{
#ifdef DEBUG
  std::cerr << "=======Clean Leaf Nodes=====" << std::endl;
#endif
  //set graph
  Graph tmp_graph;
  readGraph(tmp_graph); 
  
  VertexNameMap vertex_to_name = get(boost::vertex_name, tmp_graph);
  /*track the number of children each node has*/
  vector<int> num_of_children(numTotalNodes, 0);
  vertex_iter vi, vi_end;
  for (tie(vi, vi_end) = vertices(tmp_graph); vi != vi_end; ++vi)
  {
    int node_id = vertex_to_name[*vi];
    num_of_children.at(node_id) = boost::out_degree(*vi, tmp_graph);
  }
  
  unordered_set<unsigned> to_remove_nodes;
  
  std::vector< Vertex > topo_nodes;
  boost::topological_sort(tmp_graph, std::back_inserter(topo_nodes));
  //bottom nodes first
  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi)
  {
    int node_id = vertex_to_name[*vi];
    assert(num_of_children.at(node_id) >= 0);
    if (num_of_children.at(node_id) == 0 
      && microop.at(node_id) != IRSTOREREL
      && microop.at(node_id) != IRRET 
      && microop.at(node_id) != IRBRANCHIF 
      && microop.at(node_id) != IRBRANCHIFNOT
      && microop.at(node_id) != IRCALL)
    {
      to_remove_nodes.insert(node_id); 
      //iterate its parents
      in_edge_iter in_edge_it, in_edge_end;
      for (tie(in_edge_it, in_edge_end) = in_edges(*vi, tmp_graph); in_edge_it != in_edge_end; ++in_edge_it)
      {
        int parent_id = vertex_to_name[source(*in_edge_it, tmp_graph)];
        assert(num_of_children.at(parent_id) != 0);
        num_of_children.at(parent_id)--;
      }
    }
  }
  writeGraphWithIsolatedNodes(to_remove_nodes);
#ifdef DEBUG
  std::cerr << "=======Cleaned Leaf Nodes:" << to_remove_nodes.size() << "=====" << std::endl;
#endif
}

void Datapath::removeInductionDependence()
{
  //set graph
  //std::cerr << "=======Remove Induction Dependence=====" << std::endl;
  
#ifdef DEBUG
  std::cerr << "=======Remove Induction Dependence=====" << std::endl;
#endif
  
  Graph tmp_graph;
  readGraph(tmp_graph); 
  
  VertexNameMap vertex_to_name = get(boost::vertex_name, tmp_graph);
  EdgeNameMap edge_to_name = get(boost::edge_name, tmp_graph);
  
  unsigned num_of_edges = boost::num_edges(tmp_graph);
  std::vector<string> resultVid(numTotalNodes, "");
  initResultVid(resultVid);

  std::vector<unsigned> edge_latency(num_of_edges, 0);
  initEdgeLatency(edge_latency);
  
  int removed_edges = 0;
  
  std::vector< Vertex > topo_nodes;
  boost::topological_sort(tmp_graph, std::back_inserter(topo_nodes));
  //nodes with no incoming edges to first
  for (auto vi = topo_nodes.rbegin(); vi != topo_nodes.rend(); ++vi)
  {
    int node_id = vertex_to_name[*vi];
    string node_resultVid = resultVid.at(node_id);
    
    if (node_resultVid.find("indvars") != 0)
      continue;
    //cerr << "found an induction var " << node_id << ", " << node_resultVid << endl;   
    out_edge_iter out_edge_it, out_edge_end;
    for (tie(out_edge_it, out_edge_end) = out_edges(*vi, tmp_graph); out_edge_it != out_edge_end; ++out_edge_it)
    {
      int edge_id = edge_to_name[*out_edge_it];
      //cerr << "edge to  " << vertex_to_name[target(*out_edge_it, tmp_graph)] << endl;
      edge_latency.at(edge_id) = 0;
      removed_edges++;
    }
  }
  writeEdgeLatency(edge_latency);

#ifdef DEBUG
  std::cerr << "=======Removed Induction Edges: " << removed_edges << "=====" << std::endl;
#endif
}

void Datapath::removeAddressCalculation()
{
#ifdef DEBUG
  std::cerr << "=======Remove Address Calculation=====" << std::endl;
#endif
  //set graph
  Graph tmp_graph;
  readGraph(tmp_graph); 
  VertexNameMap vertex_to_name = get(boost::vertex_name, tmp_graph);
  EdgeNameMap edge_to_name = get(boost::edge_name, tmp_graph);
  
  unsigned num_of_edges = boost::num_edges(tmp_graph);
  
  std::vector<bool> to_remove_edges(num_of_edges, 0);
  std::vector<newEdge> to_add_edges;

  std::vector<int> edge_parid(num_of_edges, 0);
  initEdgeParID(edge_parid);

  std::vector<int> par2vid(numTotalNodes, 0);
  initParVid(par2vid, 2);

  std::vector<bool> updated(numTotalNodes, 0);
  
  int address_cal_edges = 0;

  std::vector< Vertex > topo_nodes;
  boost::topological_sort(tmp_graph, std::back_inserter(topo_nodes));
  
  //nodes with no outgoing edges go first
  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi)
  {
    int node_id = vertex_to_name[*vi];
    if (updated.at(node_id))
      continue;
    updated.at(node_id) = 1;
    int node_microop = microop.at(node_id);
    if (node_microop != IRLOADREL && node_microop != IRSTOREREL)
      continue;
    //iterate its parents
    in_edge_iter in_i, in_end;
    for (tie(in_i, in_end) = in_edges(*vi , tmp_graph); in_i != in_end; ++in_i)
    {
      int parent_id = vertex_to_name[source(*in_i, tmp_graph)];
      int parent_microop	= microop.at(parent_id);
      int edge_id = edge_to_name[*in_i];
      int load_add_edge_parid = edge_parid.at(edge_id);
      if (parent_microop == IRADD && load_add_edge_parid == 1)
      {
        address_cal_edges++;
        microop.at(parent_id) = IRINDEXADD;
        //remove the edge between the laod and the add
        to_remove_edges.at(edge_id) = 1;
        //iterate through grandparents
        in_edge_iter in_grand_i, in_grand_end;
        for(tie(in_grand_i, in_grand_end) = in_edges(source(*in_i, tmp_graph), tmp_graph); in_grand_i != in_grand_end; ++in_grand_i)
        {
          int grandparent_id = vertex_to_name[source(*in_grand_i, tmp_graph)];   
          int grandparent_microop = microop.at(grandparent_id);
          int p_edge_id = edge_to_name[*in_grand_i];
          int mul_add_edge_parid = edge_parid.at(p_edge_id);
          if (grandparent_microop == IRMUL && mul_add_edge_parid == 2)
          {
            address_cal_edges++;
            //remove the edge between the add and the mul
            to_remove_edges.at(p_edge_id) = 1;
            int grandparent_par2vid = par2vid.at(grandparent_id);
            //if the second parameter of the mul is a constant
            if (grandparent_par2vid == -1)
            {
              //iterate through grandparent's parent...
              in_edge_iter in_mul_i, in_mul_end;
              for (tie(in_mul_i, in_mul_end) = in_edges(source(*in_grand_i, tmp_graph), tmp_graph); in_mul_i != in_mul_end; ++in_mul_i)
              {
                int mul_parent = vertex_to_name[source(*in_mul_i, tmp_graph)];
                int m_edge_id = edge_to_name[*in_mul_i];
                address_cal_edges++;
                //remove the edge between the mul and its parent
                to_remove_edges.at(m_edge_id) = 1;
                //add an edge between mul's parent and the memory node
                to_add_edges.push_back( {mul_parent, node_id, -1, load_add_edge_parid, 1} );
              }
            }
          }
        }
        updated.at(parent_id) = 1;
      }
    }
  }
  int curr_num_of_edges = writeGraphWithIsolatedEdges(to_remove_edges);
  writeGraphWithNewEdges(to_add_edges, curr_num_of_edges);

#ifdef DEBUG
  std::cerr << "=======End of Address Calculation: " << address_cal_edges << "=====" << std::endl;
#endif
  cleanLeafNodes();
}

void Datapath::removeBranchEdges()
{
#ifdef DEBUG
  std::cerr << "=======Remove Branch Edges=====" << std::endl;
#endif
  //set graph
  Graph tmp_graph;
  readGraph(tmp_graph); 

  VertexNameMap vertex_to_name = get(boost::vertex_name, tmp_graph);
  EdgeNameMap edge_to_name = get(boost::edge_name, tmp_graph);
  
  unsigned num_of_edges = boost::num_edges(tmp_graph);
  std::vector<unsigned> edge_latency(num_of_edges, 0);
  initEdgeLatency(edge_latency);
  
  int branch_edges = 0;
  std::vector<bool> to_remove_edges(num_of_edges, 0);
  
  edge_iter ei, ei_end;
  for (tie(ei, ei_end) = edges(tmp_graph); ei != ei_end; ++ei)
  {
    int edge_id = edge_to_name[*ei];
    int from = vertex_to_name[source(*ei, tmp_graph)];
    int to   = vertex_to_name[target(*ei, tmp_graph)];
    int from_microop = microop.at(from);
    int to_microop = microop.at(to);
    if (is_compare_inst(from_microop) && is_branch_inst(to_microop))
    {
      to_remove_edges.at(edge_id) = 1;
      branch_edges++;
    }
    else if ( from_microop == IRSTORE || to_microop == IRSTORE 
       || from_microop == IRCONV || to_microop == IRCONV
       || from_microop == IRGETADDRESS || to_microop == IRGETADDRESS)
    {
      edge_latency.at(edge_id) = 0;
      branch_edges++;
    }
  }
  writeEdgeLatency(edge_latency);
  writeGraphWithIsolatedEdges(to_remove_edges);
#ifdef DEBUG
  std::cerr << "=======Removed Branch Edges: " << branch_edges << "=====" << std::endl;
#endif
}
void Datapath::methodGraphBuilder()
{
#ifdef DEBUG
  std::cerr << "=======Method Graph Builder=====" << std::endl;
#endif
  string call_file_name(benchName);
  call_file_name += "_method_call_graph";
  ifstream call_file;
  call_file.open(call_file_name);

  std::vector<callDep> method_dep;
  std::vector<string> node_att;
  while (!call_file.eof())
  {
    string wholeline;
    getline(call_file, wholeline);
    if (wholeline.size() == 0)
      break;
    char curr[256];
    char next[256];
    int dynamic_id, inst_id;
    sscanf(wholeline.c_str(), "%d,%[^,],%d,%[^,]\n", &dynamic_id, curr, &inst_id, next);
    
    string caller_method(curr);
    string callee_method(next);
    
    node_att.push_back(caller_method);
    method_dep.push_back({caller_method, callee_method, dynamic_id});
  }
  call_file.close();
  //write output
  ofstream edgelist_file;
  //, edge_att_file, node_att_file;

  string graph_file(benchName);
  graph_file += "_method_graph";
  edgelist_file.open(graph_file);
  edgelist_file << "digraph MethodGraph {" << std::endl;
  
  for(unsigned i = 0; i < node_att.size() ; i++)
    edgelist_file << "\"" << node_att.at(i) << "\"" << ";" << std::endl;
  
  for (auto it = method_dep.begin(); it != method_dep.end(); ++it)
  {
    edgelist_file << "\"" << it->caller  << "\"" << " -> " 
                  << "\"" << it->callee  << "\"" 
                  << " [e_inst = " << it->callInstID << "];" << std::endl;
  }
  edgelist_file << "}" << endl;
  edgelist_file.close();
}
void Datapath::methodGraphSplitter()
{
#ifdef DEBUG
  std::cerr << "=======Method Graph Splitter=====" << std::endl;
#endif
  //set graph
  MethodGraph tmp_method_graph;
  readMethodGraph(tmp_method_graph);
  
  MethodVertexNameMap vertex_to_name = get(boost::vertex_name, tmp_method_graph);
  MethodEdgeNameMap edge_to_name = get(boost::edge_name, tmp_method_graph);

  unsigned num_method_nodes = num_vertices(tmp_method_graph);
  
  std::vector<string> dynamic_methodid(numTotalNodes, "");
  initDynamicMethodID(dynamic_methodid);

  std::vector<bool> updated(numTotalNodes, 0);
  
  std::vector< MethodVertex > topo_nodes;
  boost::topological_sort(tmp_method_graph, std::back_inserter(topo_nodes));
  ofstream method_order;
  string bn(benchName);
  method_order.open(bn + "_method_order");
  
  //set graph
  unordered_map<string, edgeAtt > full_graph ;
  //FIXME
  initializeGraphInMap(full_graph);
  //bottom node first
  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi)
  {
    string method_name = vertex_to_name[*vi]; 
#ifdef DEBUG
    cerr << "current method: " << method_name << endl;
#endif    
    //only one caller for each dynamic function
    assert (boost::in_degree(*vi, tmp_method_graph) <= 1);
    if (boost::in_degree(*vi, tmp_method_graph) == 0)
      break;
    
    method_in_edge_iter in_edge_it, in_edge_end;
    tie(in_edge_it, in_edge_end) = in_edges(*vi, tmp_method_graph);
    int call_inst = edge_to_name[*in_edge_it];
#ifdef DEBUG
    cerr << "call_inst," << call_inst << endl;
#endif 
    method_order << method_name << "," <<
      vertex_to_name[source(*in_edge_it, tmp_method_graph)] << "," << call_inst << std::endl;
    std::unordered_set<int> to_split_nodes;
    
    unsigned min_node = call_inst + 1; 
    unsigned max_node = call_inst + 1;
    for (unsigned node_id = call_inst + 1; node_id < numTotalNodes; node_id++)
    {
      string node_dynamic_methodid = dynamic_methodid.at(node_id);
      if (node_dynamic_methodid.compare(method_name) != 0  || microop.at(node_id) == IRRET)
        break;
      to_split_nodes.insert(node_id);
      if (node_id > max_node)
        max_node = node_id;
    }
    cerr << "to_split_nodes," << to_split_nodes.size() << endl;
    unordered_map<string, edgeAtt> current_graph;
    auto graph_it = full_graph.begin(); 
    while (graph_it != full_graph.end())
    {
      int from, to;
      sscanf(graph_it->first.c_str(), "%d-%d", &from, &to);
      assert (from != to);
      if ( ( from < min_node && to < min_node )
        || (from > max_node && to > max_node))
      {
        graph_it++;
        continue;
      }
      //cerr << "checking edge: " << from << "," << to << "," << call_inst << endl;
      auto split_from_it = to_split_nodes.find(from);
      auto split_to_it = to_split_nodes.find(to);
      if (split_from_it == to_split_nodes.end() 
          && split_to_it == to_split_nodes.end())
      {
        //cerr << "non of them in the new graph, do nothing" << endl;
        graph_it++;
        continue;
      }
      else if (split_from_it == to_split_nodes.end() 
          && split_to_it != to_split_nodes.end())
      {
        //update the full graph: remove edge between from to to, add edge
        //between from to call_inst
        ostringstream oss;
        oss << from << "-" << call_inst;
        //cerr << "to in the new graph, update original graph, add from-call" << endl;
        if(from != call_inst && full_graph.find(oss.str()) == full_graph.end())
        {
          //cerr << "to in the new graph, update original graph, add from-call" << endl;
          full_graph[oss.str()] = {graph_it->second.varid, graph_it->second.parid, graph_it->second.latency};
          graph_it = full_graph.erase(graph_it);
        }
        else
          graph_it++;
      }
      else if (split_from_it != to_split_nodes.end() 
          && split_to_it == to_split_nodes.end())
      {
        //update the full graph: remove edge between from to to, add edge
        //between call_inst to to
        ostringstream oss;
        oss << call_inst << "-" << to;
        //cerr << "from in the new graph, update original graph, add call-to" << endl;
        if (call_inst != to && full_graph.find(oss.str()) != full_graph.end())
        {
          //cerr << "from in the new graph, update original graph, add call-to" << endl;
          full_graph[oss.str()] = {graph_it->second.varid, graph_it->second.parid, graph_it->second.latency};
          graph_it = full_graph.erase(graph_it);
        }
        else
          graph_it++;
      }
      else
      {
        //write to the new graph
        //cerr << "both from and to in the new graph, remove from-to in the original graph, add from-to in the new" << endl;
        if (current_graph.find(graph_it->first) == current_graph.end())
        {
          current_graph[graph_it->first] = {graph_it->second.varid, graph_it->second.parid, graph_it->second.latency};
        }
        graph_it = full_graph.erase(graph_it);
      }
    }
    writeGraphInMap(current_graph, bn + "_" + method_name);
    std::unordered_map<string, edgeAtt>().swap(current_graph);
    std::unordered_set<int>().swap(to_split_nodes);
  }
  
  method_order.close();
  writeGraphInMap(full_graph, bn);
#ifdef DEBUG
  std::cerr << "=======Finally the End of Method Graph Splitter=====" << std::endl;
#endif
}
void Datapath::addCallDependence()
{
#ifdef DEBUG
  std::cerr << "=======Add Call Dependence=====" << std::endl;
#endif
  //set graph
  Graph tmp_graph;
  readGraph(tmp_graph);
  
  std::vector<newEdge> to_add_edges;
  int last_call = -1;
  for(int node_id = 0; node_id < numTotalNodes; node_id++)
  {
    if (microop.at(node_id) == IRCALL)
    {
      if (last_call != -1)
        to_add_edges.push_back({last_call, node_id, -1, -1, 1});
      last_call = node_id;
    }
  }
  int num_of_edges = num_edges(tmp_graph);
  writeGraphWithNewEdges(to_add_edges, num_of_edges);

#ifdef DEBUG
  std::cerr << "=======End Add Call Dependence: " << to_add_edges.size() << "=====" << std::endl;
#endif
}

/*
 * Modify: microop.gz
 * */
void Datapath::nodeStrengthReduction()
{
#ifdef DEBUG
  std::cerr << "=======Node Strength Reduction=====" << std::endl;
#endif
  int changed_nodes = 0;
  //need par1/2
  string bn(benchName);
  std::vector<int> par1type(numTotalNodes, 0);
  std::vector<int> par2type(numTotalNodes, 0);
  std::vector<int> par1vid(numTotalNodes, 0);
  std::vector<int> par2vid(numTotalNodes, 0);

  initParType(par1type, 1);
  initParType(par2type, 2);
  initParVid(par1vid, 1);
  initParVid(par2vid, 2);
  
  for(unsigned node_id = 0; node_id < numTotalNodes; node_id++)
  {
    int node_microop = microop.at(node_id);
    if (node_microop != IRMUL && node_microop != IRDIV)
      continue;
    int node_par1type = par1type.at(node_id);
    int node_par2type = par2type.at(node_id);
    int node_par1vid = par1vid.at(node_id);
    int node_par2vid = par2vid.at(node_id);
    
    //if both operands are integer/unsigned integer, and at least one of the
    //operands is constant (not a variable)
    if ( (is_integer(node_par1type) || is_unsigned_integer(node_par1type)) 
      && (is_integer(node_par2type) || is_unsigned_integer(node_par2type)) 
      && (node_par1vid == -1 || node_par2vid == -1 ) )
    {
      //found mul/div nodes to be replaced
      if (node_microop == IRMUL)
        microop.at(node_id) =  IRSHL;
      else
        microop.at(node_id) =  IRSHR;
      changed_nodes++;
    }
  }
#ifdef DEBUG
  std::cerr << "=======ChangedNodes: " << changed_nodes << "=====" << std::endl;
#endif
}
/*
 * Modify: benchName_membase.gz
 * */
void Datapath::scratchpadPartition()
{
#ifdef DEBUG
  std::cerr << "=======ScratchPad Partition=====" << std::endl;
#endif
  string bn(benchName);
  std::vector<pair<unsigned, unsigned> > address(numTotalNodes, make_pair(0,0));
  cerr << "before init address" << endl;
  initAddressAndSize(address);
  cerr << "after init address" << endl;

  std::vector<unsigned> orig_base(numTotalNodes, 0);
  read_gzip_unsigned_file(bn + "_membase.gz", numTotalNodes, orig_base);
  
  //read the partition config file to get the address range
  // <base addr, <type, part_factor> > 
  cerr << "before read partition" << endl;
  std::unordered_map<unsigned, partitionEntry> part_config;
  readPartitionConfig(part_config);
  cerr << "after read partition" << endl;
  //set scratchpad
  for(auto it = part_config.begin(); it!= part_config.end(); ++it)
  {
    unsigned base_addr = it->first;
    unsigned size = it->second.array_size;
    unsigned p_factor = it->second.part_factor;
#ifdef DEBUG
    cerr << base_addr << "," << size << "," << p_factor << endl;
#endif
    for ( unsigned i = 0; i < p_factor ; i++)
    {
      ostringstream oss;
      oss << base_addr << "-" << i;
      scratchpad->setScratchpad(oss.str(), size);
    }
  }
#ifdef DEBUG
  cerr << "End Setting Scratchpad" << endl;
#endif
  for(unsigned node_id = 0; node_id < numTotalNodes; node_id++)
  {
    unsigned node_base = orig_base.at(node_id);
    if (node_base == 0) //not memory operation
      continue;
    auto part_it = part_config.find(node_base);
    if (part_it == part_config.end())
    {
      ostringstream oss;
      oss << node_base;
      baseAddress.at(node_id) = oss.str();
    }
    else
    {
      string p_type = part_it->second.type;
      assert((!p_type.compare("block")) || (!p_type.compare("cyclic")));
      unsigned num_of_elements = part_it->second.array_size;
      unsigned p_factor = part_it->second.part_factor;
      unsigned abs_addr = address.at(node_id).first;
      unsigned data_size = address.at(node_id).second;
      unsigned rel_addr = (abs_addr - node_base ) / data_size; 
      if (!p_type.compare("block"))  //block partition
      {
        ostringstream oss;
        unsigned num_of_elements_in_2 = next_power_of_two(num_of_elements);
        oss << node_base << "-" << (int) (rel_addr / ceil (num_of_elements_in_2  / p_factor)) ;
        baseAddress.at(node_id) = oss.str();
      }
      else // (!p_type.compare("cyclic")), cyclic partition
      {
        ostringstream oss;
        oss << node_base << "-" << (rel_addr) % p_factor;
        baseAddress.at(node_id) = oss.str();
      }
    }
  }
  //FIXME
  //write_gzip_string_file(bn + "_new_membase.gz", numTotalNodes, baseAddress);
}
//called in the end of the whole flow
void Datapath::dumpStats()
{
  writeFinalLevel();
  writeGlobalIsolated();
  writePerCycleActivity();
  writeRegStats();
}

//localOptimizationFunctions
void Datapath::setGraphName(string graph_name, int min)
{
  graphName = (char*)graph_name.c_str();
  cerr << "setting minNode " << min << endl;
  minNode = min;
}

void Datapath::optimizationPass()
{
  //loop unrolling
  loopUnrolling();
  //remove shared loads
  removeSharedLoads();
  //store buffer
  storeBuffer();
  //remove repeat stores
  removeRepeatedStores();
  //tree height reduction
  //treeHeightReduction();
  
  writeMicroop(microop);
}

void Datapath::loopUnrolling()
{
#ifdef DEBUG
  std::cerr << "=======Loop Unrolling=====" << std::endl;
#endif
  //set graph
  Graph tmp_graph;
  readGraph(tmp_graph); 
  
  VertexNameMap vertex_to_name = get(boost::vertex_name, tmp_graph);
  EdgeNameMap edge_to_name = get(boost::edge_name, tmp_graph);

  //unordered_map
  //custom allocator, boost::slab allocator 
  std::unordered_map<int, Vertex> name_to_vertex;
  BGL_FORALL_VERTICES(v, tmp_graph, Graph)
    name_to_vertex[get(boost::vertex_name, tmp_graph, v)] = v;
  
  unsigned num_of_edges = boost::num_edges(tmp_graph);
  unsigned num_of_nodes = boost::num_vertices(tmp_graph);
  
  std::vector<int> instid(num_of_nodes, 0);
  initInstID(instid);

  std::vector<int> methodid(num_of_nodes, 0);
  initMethodID(methodid);
  
  std::vector<unsigned> edge_latency(num_of_edges, 0);
  initEdgeLatency(edge_latency);
  
  std::vector<bool> tmp_isolated(num_of_nodes, 0);
  vertex_iter vi, vi_end;
  for (tie(vi, vi_end) = vertices(tmp_graph); vi != vi_end; ++vi)
  {
    if (boost::degree(*vi, tmp_graph) == 0)
      tmp_isolated.at(vertex_to_name[*vi]) = 1;
  }
  
  std::unordered_map<string, pair<int, int> > unrolling_config;
  readUnrollingConfig(unrolling_config);

  int add_unrolling_edges = 0;
  
  ofstream boundary;
  string file_name(graphName);
  file_name += "_loop_boundary";
  boundary.open(file_name.c_str());
  bool first = 0;
  //cerr << "minNode, num_of_nodes" << minNode << "," << num_of_nodes << endl;
  for(unsigned node_id = minNode; node_id < num_of_nodes; node_id++)
  {
    if(tmp_isolated.at(node_id))
      continue;
    if (!first)
    {
      first = 1;
      boundary << node_id << endl;
    }
    int node_instid = instid.at(node_id);
    int node_methodid = methodid.at(node_id);
    ostringstream oss;
    oss << node_methodid << "-" << node_instid;
    auto unroll_it = unrolling_config.find(oss.str());
    if (unroll_it == unrolling_config.end())
      continue;
    int factor = unroll_it->second.first;
    unroll_it->second.second++;
    //time to roll
    if (unroll_it->second.second % factor == 0)
    {
      boundary << node_id << endl;
      microop.at(node_id) = IRINDEXADD;
      
      Vertex node = name_to_vertex[node_id];
      out_edge_iter out_edge_it, out_edge_end;
      for (tie(out_edge_it, out_edge_end) = out_edges(node, tmp_graph); out_edge_it != out_edge_end; ++out_edge_it)
      {
        int child_id = vertex_to_name[target(*out_edge_it, tmp_graph)];
        int child_instid = instid.at(child_id);
        int child_methodid = methodid.at(child_id);
        if ((child_instid == node_instid) 
           && (child_methodid == node_methodid))
        {
          int edge_id = edge_to_name[*out_edge_it];
          edge_latency[edge_id] = 1;
          microop.at(child_id) = IRINDEXADD;
          add_unrolling_edges++;
        }
      }
    }
    else if (microop.at(node_id) != IRINDEXADD)
      microop.at(node_id) = IRINDEXSTORE;
  }
  boundary << num_of_nodes << endl;
  boundary.close();
#ifdef DEBUG
  std::cerr << "=======End Loop Unrolling: "<< add_unrolling_edges << " =====" << std::endl;
#endif
}

void Datapath::removeSharedLoads()
{
#ifdef DEBUG
  std::cerr << "=======Remove Shared Loads=====" << std::endl;
#endif
  //set graph
  Graph tmp_graph;
  readGraph(tmp_graph); 
  VertexNameMap vertex_to_name = get(boost::vertex_name, tmp_graph);
  EdgeNameMap edge_to_name = get(boost::edge_name, tmp_graph);
  
  unsigned num_of_edges = boost::num_edges(tmp_graph);
  unsigned num_of_nodes = boost::num_vertices(tmp_graph);
  
  std::map<int, Vertex> name_to_vertex;
  BGL_FORALL_VERTICES(v, tmp_graph, Graph)
    name_to_vertex[get(boost::vertex_name, tmp_graph, v)] = v;
  
  std::vector<unsigned> address(num_of_nodes, 0);
  initAddress(address);
  
  std::vector<unsigned> edge_latency(num_of_edges, 0);
  std::vector<int> edge_parid(num_of_edges, 0);
  std::vector<int> edge_varid(num_of_edges, 0);

  initEdgeLatency(edge_latency);
  initEdgeParID(edge_parid);
  initEdgeVarID(edge_varid);
  
  std::vector<int> boundary;
  string file_name(graphName);
  file_name += "_loop_boundary";
  read_file(file_name, boundary);

  std::vector<bool> tmp_isolated(num_of_nodes, 0);
  vertex_iter vi, vi_end;
  for (tie(vi, vi_end) = vertices(tmp_graph); vi != vi_end; ++vi)
  {
    if (boost::degree(*vi, tmp_graph) == 0)
      tmp_isolated.at(vertex_to_name[*vi]) = 1;
  }
  
  std::vector<bool> to_remove_edges(num_of_edges, 0);
  std::vector<newEdge> to_add_edges;

  int shared_loads = 0;
  unsigned boundary_id = 0;
  int curr_boundary = boundary.at(boundary_id);
  
  int node_id = minNode;
  while ( (unsigned)node_id < num_of_nodes)
  {
    std::unordered_map<unsigned, int> address_loaded;
    while (node_id < curr_boundary &&  (unsigned) node_id < num_of_nodes)
    {
      if(tmp_isolated.at(node_id))
      {
        node_id++;
        continue;
      }
      int node_microop = microop.at(node_id);
      unsigned node_address = address.at(node_id);
      auto addr_it = address_loaded.find(node_address);
      if (is_store_op(node_microop) && addr_it != address_loaded.end())
        address_loaded.erase(addr_it);
      else if (is_load_op(node_microop))
      {
        if (addr_it == address_loaded.end())
          address_loaded[node_address] = node_id;
        else
        {
          shared_loads++;
          int prev_load = addr_it->second;
          //iterate throught its children
          Vertex node = name_to_vertex[node_id];
          
          out_edge_iter out_edge_it, out_edge_end;
          for (tie(out_edge_it, out_edge_end) = out_edges(node, tmp_graph); out_edge_it != out_edge_end; ++out_edge_it)
          {
            int child_id = vertex_to_name[target(*out_edge_it, tmp_graph)];
            int edge_id = edge_to_name[*out_edge_it];
            std::pair<Edge, bool> existed;
            existed = edge(name_to_vertex[prev_load], name_to_vertex[node_id], tmp_graph);
            if (existed.second == false)
              to_add_edges.push_back({prev_load, child_id, edge_varid.at(edge_id), edge_parid.at(edge_id), edge_latency.at(edge_id)});
            to_remove_edges[edge_id] = 1;
          }
          
          //iterate through its parents
          in_edge_iter in_edge_it, in_edge_end;
          for (tie(in_edge_it, in_edge_end) = in_edges(node, tmp_graph); in_edge_it != in_edge_end; ++in_edge_it)
          {
            int parent_id = vertex_to_name[source(*in_edge_it, tmp_graph)];
            if (microop.at(parent_id) == IRADD ||
                microop.at(parent_id) == IRINDEXADD ||
                microop.at(parent_id) == IRINDEXSHIFT)
              microop.at(parent_id) = IRINDEXSTORE;
          }
        }
      }
      node_id++;
    }
    ++boundary_id;
    if (boundary_id == boundary.size() )
      break;
    curr_boundary = boundary.at(boundary_id);
  }
  edge_latency.clear();
  edge_parid.clear();
  edge_varid.clear();
  int curr_num_of_edges = writeGraphWithIsolatedEdges(to_remove_edges);
  writeGraphWithNewEdges(to_add_edges, curr_num_of_edges);
#ifdef DEBUG
  std::cerr << "=======End of Shared Loads: " << shared_loads << "=====" << std::endl;
#endif
  cleanLeafNodes();
}

void Datapath::storeBuffer()
{
#ifdef DEBUG
  std::cerr << "=======Store Buffer=====" << std::endl;
#endif
  //set graph
  Graph tmp_graph;
  readGraph(tmp_graph); 
  EdgeNameMap edge_to_name = get(boost::edge_name, tmp_graph);
  VertexNameMap vertex_to_name = get(boost::vertex_name, tmp_graph);
  
  std::map<int, Vertex> name_to_vertex;
  BGL_FORALL_VERTICES(v, tmp_graph, Graph)
    name_to_vertex[get(boost::vertex_name, tmp_graph, v)] = v;
  
  unsigned num_of_edges = boost::num_edges(tmp_graph);
  unsigned num_of_nodes = boost::num_vertices(tmp_graph);
  
  std::vector<int> edge_parid(num_of_edges, 0);
  std::vector<int> edge_varid(num_of_edges, 0);
  std::vector<unsigned> edge_latency(num_of_edges, 0);
  
  initEdgeLatency(edge_latency);
  initEdgeParID(edge_parid);
  initEdgeVarID(edge_varid);
  
  std::vector<int> boundary;
  string file_name(graphName);
  file_name += "_loop_boundary";
  read_file(file_name, boundary);
  
  std::vector<bool> tmp_isolated(num_of_nodes, 0);
  vertex_iter vi, vi_end;
  for (tie(vi, vi_end) = vertices(tmp_graph); vi != vi_end; ++vi)
  {
    if (boost::degree(*vi, tmp_graph) == 0)
      tmp_isolated.at(vertex_to_name[*vi]) = 1;
  }
  
  std::vector<bool> to_remove_edges(num_of_edges, 0);
  std::vector<newEdge> to_add_edges;

  int buffered_stores = 0;
  unsigned boundary_id = 0;
  int curr_boundary = boundary.at(boundary_id);
  
  int node_id = minNode;
  while (node_id < num_of_nodes)
  {
    while (node_id < curr_boundary && node_id < num_of_nodes)
    {
      if(tmp_isolated.at(node_id))
      {
        node_id++;
        continue;
      }
      int node_microop = microop.at(node_id);
      if (is_store_op(node_microop))
      {
        Vertex node = name_to_vertex[node_id];
        out_edge_iter out_edge_it, out_edge_end;
        std::vector<unsigned> load_child;
        for (tie(out_edge_it, out_edge_end) = out_edges(node, tmp_graph); out_edge_it != out_edge_end; ++out_edge_it)
        {
          int child_id = vertex_to_name[target(*out_edge_it, tmp_graph)];
          int child_microop = microop.at(child_id);
          if (is_load_op(child_microop))
          {
            if (child_id >= (unsigned)curr_boundary )
              continue;
            else
              load_child.push_back(child_id);
          }
        }
        
        if (load_child.size() > 0)
        {
          buffered_stores++;
          
          in_edge_iter in_edge_it, in_edge_end;
          int store_parent = num_of_nodes;
          for (tie(in_edge_it, in_edge_end) = in_edges(node, tmp_graph); in_edge_it != in_edge_end; ++in_edge_it)
          {
            int edge_id = edge_to_name[*in_edge_it];
            int parent_id = vertex_to_name[source(*in_edge_it, tmp_graph)];
            int parid = edge_parid.at(edge_id);
            //parent node that generates value
            if (parid == 3)
            {
              store_parent = parent_id;
              break;
            }
          }
          
          if (store_parent != num_of_nodes)
          {
            for (unsigned i = 0; i < load_child.size(); ++i)
            {
              unsigned load_id = load_child.at(i);
              
              Vertex load_node = name_to_vertex[load_id];
              
              out_edge_iter out_edge_it, out_edge_end;
              for (tie(out_edge_it, out_edge_end) = out_edges(load_node, tmp_graph); out_edge_it != out_edge_end; ++out_edge_it)
              {
                int edge_id = edge_to_name[*out_edge_it];
                int child_id = vertex_to_name[target(*out_edge_it, tmp_graph)];
                to_remove_edges.at(edge_id) = 1;
                to_add_edges.push_back({store_parent, child_id, edge_varid.at(edge_id), edge_parid.at(edge_id), edge_latency.at(edge_id)});
              }
              
              for (tie(in_edge_it, in_edge_end) = in_edges(load_node, tmp_graph); in_edge_it != in_edge_end; ++in_edge_it)
              {
                int edge_id = edge_to_name[*in_edge_it];
                to_remove_edges.at(edge_id) = 1;
              }
            }
          }
        }
      }
      ++node_id; 
    }
    ++boundary_id;
    if (boundary_id == boundary.size() )
      break;
    curr_boundary = boundary.at(boundary_id);
  }
  edge_latency.clear();
  edge_parid.clear();
  edge_varid.clear();
  int curr_num_of_edges = writeGraphWithIsolatedEdges(to_remove_edges);
  writeGraphWithNewEdges(to_add_edges, curr_num_of_edges);
#ifdef DEBUG
  std::cerr << "=======End of Buffered Stores: " << buffered_stores << "=====" << std::endl;
#endif
  cleanLeafNodes();
}

void Datapath::removeRepeatedStores()
{
#ifdef DEBUG
  std::cerr << "=======Remove Repeated Stores=====" << std::endl;
#endif
  //set graph
  Graph tmp_graph;
  readGraph(tmp_graph); 
  
  std::map<int, Vertex> name_to_vertex;
  BGL_FORALL_VERTICES(v, tmp_graph, Graph)
    name_to_vertex[get(boost::vertex_name, tmp_graph, v)] = v;
  
  VertexNameMap vertex_to_name = get(boost::vertex_name, tmp_graph);
  
  unsigned num_of_nodes = boost::num_vertices(tmp_graph);

  std::vector<unsigned> address(num_of_nodes, 0);
  initAddress(address);
  
  std::vector<bool> tmp_isolated(num_of_nodes, 0);
  vertex_iter vi, vi_end;
  for (tie(vi, vi_end) = vertices(tmp_graph); vi != vi_end; ++vi)
  {
    if (boost::degree(*vi, tmp_graph) == 0)
      tmp_isolated.at(vertex_to_name[*vi]) = 1;
  }

  std::vector<int> boundary;
  string file_name(graphName);
  file_name += "_loop_boundary";
  read_file(file_name, boundary);
  if (boundary.size() < 1)
    return;
  unordered_set<unsigned> to_remove_nodes;

  int shared_stores = 0;
  int boundary_id = boundary.size() - 1;
  int node_id = num_of_nodes -1;
  int next_boundary = boundary.at(boundary_id - 1);
  while (node_id >=minNode )
  {
    unordered_map<unsigned, int> address_store_map;
    while (node_id >= next_boundary && node_id >= minNode)
    {
      if (tmp_isolated.at(node_id) )
      {
        --node_id;
        continue;
      }
      int node_microop = microop.at(node_id);
      if (is_store_op(node_microop))
      {
        unsigned node_address = address.at(node_id);
        auto addr_it = address_store_map.find(node_address);
        if (addr_it == address_store_map.end())
          address_store_map[node_address] = node_id;
        else
        {
          //remove this store
          //if it has children, ignore it
          Vertex node = name_to_vertex[node_id];
          
          if (boost::out_degree(node, tmp_graph)== 0)
          {
            to_remove_nodes.insert(node_id);
            shared_stores++;
          }
        }
      }
      node_id--; 
    }
    if (node_id < next_boundary)
    {
      --boundary_id;
      if (boundary_id == -1)
        break;
      next_boundary = boundary.at(boundary_id);
    }
  }
  writeGraphWithIsolatedNodes(to_remove_nodes);
#ifdef DEBUG
  std::cerr << "=======End of Remove Repeated Stores " << shared_stores << "=====" << std::endl;
#endif
  cleanLeafNodes();
}
/*
void Datapath::treeHeightReduction()
{
#ifdef DEBUG
  std::cerr << "=======Remove Shared Loads=====" << std::endl;
#endif
  //set graph
  Graph tmp_graph;
  readGraph(tmp_graph); 
  unsigned num_of_nodes = boost::num_vertices(tmp_graph);
  unsigned num_of_edges = boost::num_edges(tmp_graph);
   
  std::vector<int> par1type(numTotalNodes, 0);
  std::vector<int> par2type(numTotalNodes, 0);
  std::vector<int> par4type(numTotalNodes, 0);
  std::vector<int> par1vid(numTotalNodes, 0);
  std::vector<int> par2vid(numTotalNodes, 0);
  std::vector<int> par4vid(numTotalNodes, 0);
  std::vector<string> par1value(numTotalNodes, "");
  std::vector<string> par2value(numTotalNodes, "");
  std::vector<string> par4value(numTotalNodes, "");
  
  initParType(par1type, 1);
  initParType(par2type, 2);
  initParType(par4type, 4);
  initParVid(par1vid, 1);
  initParVid(par2vid, 2);
  initParVid(par4vid, 4);
  initParValue(par1value, 1);
  initParValue(par2value, 2);
  initParValue(par4value, 4);
  
  vector<int> new_par1vid(par1vid);
  vector<int> new_par2vid(par2vid);
  vector<int> new_par4vid(par4vid);
  vector<int> new_par1type(par1type);
  vector<int> new_par2type(par2type);
  vector<int> new_par4type(par4type);
  vector<string> new_par1value(par1value);
  vector<string> new_par2value(par2value);
  vector<string> new_par4value(par4value);
  
  std::vector<int> boundary;
  string file_name(graphName);
  file_name += "_loop_boundary";
  read_file(file_name, boundary);
  if (boundary.size() < 1)
    return;
  
  std::vector<bool> tmp_isolated(num_of_nodes, 0);
  //isolated_nodes(tmp_g, num_of_nodes, tmp_isolated);
  vertex_iter vi, vi_end;
  for (tie(vi, vi_end) = vertices(tmp_graph); vi != vi_end; ++vi)
  {
    if (boost::degree(*vi, tmp_graph) == 0)
      tmp_isolated.at(vertex_to_name[*vi]) = 1;
  }
  
  std::vector<bool> to_remove_edges(num_of_edges, 0);
  std::vector<newEdge> to_add_edges;
  
  unsigned boundary_id = 1;
  //FIXME
  int used_vid = max_value(par4vid, 0, num_of_vertices);

  while (boundary_id <boundary.size() - 1)
  {
    unsigned end_inst = boundary.at(boundary_id);
    unsigned start_inst = boundary.at(boundary_id -1);
    if (start_inst >= num_of_nodes || end_inst>= num_of_nodes )
      break;
    std::vector<int> v_sort;
    //FIXME
    partial_topological_sort(v_level, start_inst, end_inst, v_sort, tmp_isolated);
    
    vector<bool> updated(end_inst - start_inst, 0);
    for (unsigned i = 0; i < v_sort.size(); ++i)
    {
      int node_id = v_sort.at(i);
      if (tmp_isolated.at(node_id))
        continue;
      if (updated.at(node_id - start_inst))
        continue;
      updated.at(node_id - start_inst ) = 1;
      
      list<int> nodes;
      vector<int> leaves;
      vector<int> leaves_rank;
      vector<pair<int, int>> temp_to_remove_edges;
      if (is_associative(microop.at(node_id)))
      {
        findAllAssociativeChildren(tmp_g, node_id, start_inst, nodes,
          leaves, leaves_rank, temp_to_remove_edges);
      }
      if (nodes.size() < 3 )
        continue;
      
      unordered_map<unsigned, unsigned> rank_map;
      for (auto edge_it = temp_to_remove_edges.begin(); 
           edge_it != temp_to_remove_edges.end(); ++edge_it)
      {
        int source = edge_it->first;
        int sink = edge_it->second;
        int edge_id = check_edgeid(source, sink, tmp_g);
        to_remove_edges[edge_id] = 1;
      }
      auto leaf_it = leaves.begin();
      auto leaf_rank_it = leaves_rank.begin();
      
      while (leaf_it != leaves.end())
      {
        if (*leaf_rank_it == 0)
          rank_map[*leaf_it] = *leaf_rank_it;
        else
          rank_map[*leaf_it] = num_of_nodes;
        //cerr << *leaf_it << ", rank, " << rank_map[*leaf_it] << endl;
        ++leaf_it;
        ++leaf_rank_it;
      }
      //reconstruct the rest of the balanced tree
      auto node_it = nodes.begin();
      //cerr << "nodes size, " << nodes.size() << endl;
      while (node_it != (--nodes.end()))
      {
        unsigned node1 = num_of_nodes;
        unsigned node2 = num_of_nodes;
        int rank1 = num_of_nodes;
        int rank2 = num_of_nodes;
        unsigned min_rank = num_of_nodes;
        for (auto it = rank_map.begin(); it != rank_map.end(); ++it)
        {
          if (it->second < min_rank)
          {
            node1 = it->first;
            rank1 = it->second;
            min_rank = rank1;
          }
        }
        min_rank = num_of_nodes;
        for (auto it = rank_map.begin(); it != rank_map.end(); ++it)
        {
          if ((it->first != node1) && (it->second < min_rank))
          {
            node2 = it->first;
            rank2 = it->second;
            min_rank = rank2;
          }
        }
        //cerr << "new edge: " << node1 << "," << *node_it << endl;
        assert((node1 != num_of_nodes) && (node2 != num_of_nodes));
        updated.at(*node_it - start_inst) = 1;
        
        //new_par1id.at(*node_it) = par4vid.at(node1);
        //new_par1value.at(*node_it) = par4value.at(node1);
        //new_par1type.at(*node_it) = par1type.at(node1);
        
        newEdge *p = new newEdge[1];
        p->from = node1;
        p->to = *node_it;
        //FIXME
        p->varid = par4vid.at(node1);
        p->parid = 1;
        p->latency = 1;
        to_add_edges.push_back(*p);
        delete p;

        //new_par2id.at(*node_it) = par4vid.at(node2);
        //new_par2value.at(*node_it) = par4value.at(node2);
        //new_par2type.at(*node_it) = par1type.at(node2);
        
        newEdge *p = new newEdge[1];
        p->from = node2;
        p->to = *node_it;
        //FIXME
        p->varid = par4vid.at(node2);
        p->parid = 2;
        p->latency = 1;
        to_add_edges.push_back(*p);
        delete p;
        
        //new_par4id.at(*node_it) = ++used_vid;
        //new_par4value.at(*node_it) = "";
        //new_par4type.at(*node_it) = par1type.at(*node_it);
        
        //place the new node in the map, remove the two old nodes
        rank_map.erase(node1);
        rank_map.erase(node2);
        int temp_rank = max(rank1, rank2) + 1;
        rank_map[*node_it] = temp_rank;
        
        ++node_it;
        
      }
      //only two nodes left in the rank map, set them as chidren of the head
      //node
      assert(rank_map.size() == 2);
      int node1 = rank_map.begin()->first;
      int node2 = (++rank_map.begin())->first;
      int rank1 = rank_map.begin()->second;
      int rank2 = (++rank_map.begin())->second;

      updated.at(*node_it - start_inst) = 1;
      
      new_par1id.at(*node_it) = par4vid.at(node1);
      new_par1value.at(*node_it) = par4value.at(node1);
      new_par1type.at(*node_it) = par1type.at(node1);
      
      newEdge *p = new newEdge[1];
      p->from = node1;
      p->to = *node_it;
      //FIXME
      p->varid = par4vid.at(node1);
      p->parid = 1;
      p->latency = 1;
      to_add_edges.push_back(*p);
      delete p;

      //new_par2id.at(*node_it) = par4vid.at(node2);
      //new_par2value.at(*node_it) = par4value.at(node2);
      //new_par2type.at(*node_it) = par1type.at(node2);
      
      newEdge *p = new newEdge[1];
      p->from = node2;
      p->to = *node_it;
      p->varid = par4vid.at(node2);
      p->parid = 2;
      p->latency = 1;
      to_add_edges.push_back(*p);
      delete p;
      
      //new_par4id.at(*node_it) = par4vid.at(node_id);
      //new_par4value.at(*node_it) = par4value.at(node_id);
      //new_par4type.at(*node_it) = par4type.at(node_id);
      
      rank_map.erase(node1);
      rank_map.erase(node2);
      
      int temp_rank = max(rank1, rank2) + 1;
      rank_map[*node_it] = temp_rank;
    }
    boundary_id++;
  }
  int curr_num_of_edges = writeGraphWithIsolatedEdges(to_remove_edges);
  writeGraphWithNewEdges(to_add_edges, curr_num_of_edges);

  //writeParType(new_par1type, 1);
  //writeParType(new_par2type, 2);
  //writeParType(new_par4type, 4);
  //writeParVid(new_par1vid, 1);
  //writeParVid(new_par2vid, 2);
  //writeParVid(new_par4vid, 4);
  //writeParValue(new_par1value, 1);
  //writeParValue(new_par2value, 2);
  //writeParValue(new_par4value, 4);
  
  cleanLeafNodes();
#ifdef DEBUG
  std::cerr << "=======End of Tree Height Reduction=====" << std::endl;
#endif
}

*/
//readWriteGraph
int Datapath::writeGraphWithNewEdges(std::vector<newEdge> &to_add_edges, int curr_num_of_edges)
{
  string gn(graphName);
  string graph_file, edge_varid_file, edge_parid_file, edge_latency_file;
  graph_file = gn + "_graph";
  edge_varid_file = gn + "_edgevarid.gz";
  edge_parid_file = gn + "_edgeparid.gz";
  edge_latency_file = gn + "_edgelatency.gz";
  
  ifstream orig_graph;
  ofstream new_graph;
  gzFile new_edgelatency, new_edgeparid, new_edgevarid;
  
  orig_graph.open(graph_file.c_str());
  std::filebuf *pbuf = orig_graph.rdbuf();
  std::size_t size = pbuf->pubseekoff(0, orig_graph.end, orig_graph.in);
  pbuf->pubseekpos(0, orig_graph.in);
  char *buffer = new char[size];
  pbuf->sgetn (buffer, size);
  orig_graph.close();
  
  new_graph.open(graph_file.c_str());
  new_graph.write(buffer, size);
  delete[] buffer;

  long pos = new_graph.tellp();
  new_graph.seekp(pos-2);

  new_edgelatency = gzopen(edge_latency_file.c_str(), "a");
  new_edgeparid = gzopen(edge_parid_file.c_str(), "a");
  new_edgevarid = gzopen(edge_varid_file.c_str(), "a");
  
  int new_edge_id = curr_num_of_edges;
  
  for(auto it = to_add_edges.begin(); it != to_add_edges.end(); ++it)
  {
    new_graph << it->from << " -> " 
              << it->to
              << " [e_id = " << new_edge_id << "];" << endl;
    new_edge_id++;
    gzprintf(new_edgelatency, "%d\n", it->latency);
    gzprintf(new_edgeparid, "%d\n", it->parid);
    gzprintf(new_edgevarid, "%d\n", it->varid);
  }
  new_graph << "}" << endl;
  new_graph.close();
  gzclose(new_edgelatency);
  gzclose(new_edgeparid);
  gzclose(new_edgevarid);

  return new_edge_id;
}
int Datapath::writeGraphWithIsolatedNodes(std::unordered_set<unsigned> &to_remove_nodes)
{
  Graph tmp_graph;
  readGraph(tmp_graph);
  
  unsigned num_of_edges = num_edges(tmp_graph);
  unsigned num_of_nodes = num_vertices(tmp_graph);
  
  std::vector<unsigned> edge_latency(num_of_edges, 0);
  std::vector<int> edge_parid(num_of_edges, 0);
  std::vector<int> edge_varid(num_of_edges, 0);

  initEdgeLatency(edge_latency);
  initEdgeParID(edge_parid);
  initEdgeVarID(edge_varid);
  
  VertexNameMap vertex_to_name = get(boost::vertex_name, tmp_graph);
  EdgeNameMap edge_to_name = get(boost::edge_name, tmp_graph);

  ofstream new_graph;
  gzFile new_edgelatency, new_edgeparid, new_edgevarid;
  
  string gn(graphName);
  string graph_file, edge_varid_file, edge_parid_file, edge_latency_file;
  graph_file = gn + "_graph";
  edge_varid_file = gn + "_edgevarid.gz";
  edge_parid_file = gn + "_edgeparid.gz";
  edge_latency_file = gn + "_edgelatency.gz";
  
  new_graph.open(graph_file.c_str());
  new_edgelatency = gzopen(edge_latency_file.c_str(), "a");
  new_edgeparid = gzopen(edge_parid_file.c_str(), "a");
  new_edgevarid = gzopen(edge_varid_file.c_str(), "a");
  
  new_graph << "digraph DDDG {" << std::endl;

  for (unsigned node_id = 0; node_id < numTotalNodes; node_id++)
     new_graph << node_id << ";" << std::endl; 
  
  edge_iter ei, ei_end;
  int new_edge_id = 0;
  for (tie(ei, ei_end) = edges(tmp_graph); ei != ei_end; ++ei)
  {
    int edge_id = edge_to_name[*ei];
    int from = vertex_to_name[source(*ei, tmp_graph)];
    int to   = vertex_to_name[target(*ei, tmp_graph)];
    if (to_remove_nodes.find(from) != to_remove_nodes.end() 
     || to_remove_nodes.find(to) != to_remove_nodes.end())
      continue;

    //FIXME   
    new_graph << from << " -> " 
              << to
              << " [e_id = " << new_edge_id << "];" << endl;
    new_edge_id++;
    gzprintf(new_edgelatency, "%d\n", edge_latency.at(edge_id) );
    gzprintf(new_edgeparid, "%d\n", edge_parid.at(edge_id) );
    gzprintf(new_edgevarid, "%d\n", edge_varid.at(edge_id));
  }

  new_graph << "}" << endl;
  new_graph.close();
  gzclose(new_edgelatency);
  gzclose(new_edgeparid);
  gzclose(new_edgevarid);
  
  return new_edge_id;
}
int Datapath::writeGraphWithIsolatedEdges(std::vector<bool> &to_remove_edges)
{
#ifdef DDEBUG
  std::cerr << "=======Write Graph With Isolated Edges=====" << std::endl;
#endif
  Graph tmp_graph;
  readGraph(tmp_graph);
  
  unsigned num_of_edges = num_edges(tmp_graph);
  unsigned num_of_nodes = num_vertices(tmp_graph);

  std::vector<unsigned> edge_latency(num_of_edges, 0);
  std::vector<int> edge_parid(num_of_edges, 0);
  std::vector<int> edge_varid(num_of_edges, 0);

  initEdgeLatency(edge_latency);
  initEdgeParID(edge_parid);
  initEdgeVarID(edge_varid);
  
  VertexNameMap vertex_to_name = get(boost::vertex_name, tmp_graph);
  EdgeNameMap edge_to_name = get(boost::edge_name, tmp_graph);

  ofstream new_graph;
  gzFile new_edgelatency, new_edgeparid, new_edgevarid;
  
  string gn(graphName);
  string graph_file, edge_varid_file, edge_parid_file, edge_latency_file;
  graph_file = gn + "_graph";
  edge_varid_file = gn + "_edgevarid.gz";
  edge_parid_file = gn + "_edgeparid.gz";
  edge_latency_file = gn + "_edgelatency.gz";
  
  new_graph.open(graph_file.c_str());
  new_edgelatency = gzopen(edge_latency_file.c_str(), "a");
  new_edgeparid = gzopen(edge_parid_file.c_str(), "a");
  new_edgevarid = gzopen(edge_varid_file.c_str(), "a");

  new_graph << "digraph DDDG {" << std::endl;

  for (unsigned node_id = 0; node_id < num_of_nodes; node_id++)
     new_graph << node_id << ";" << std::endl; 
  
  edge_iter ei, ei_end;
  int new_edge_id = 0;
  for (tie(ei, ei_end) = edges(tmp_graph); ei != ei_end; ++ei)
  {
    int edge_id = edge_to_name[*ei];
    if (to_remove_edges.at(edge_id))
      continue;
    new_graph << vertex_to_name[source(*ei, tmp_graph)] << " -> " 
              << vertex_to_name[target(*ei, tmp_graph)] 
              << " [e_id = " << new_edge_id << "];" << endl;
    new_edge_id++;
    gzprintf(new_edgelatency, "%d\n", edge_latency.at(edge_id) );
    gzprintf(new_edgeparid, "%d\n", edge_parid.at(edge_id) );
    gzprintf(new_edgevarid, "%d\n", edge_varid.at(edge_id));
  }
  
  new_graph << "}" << endl;
  new_graph.close();
  gzclose(new_edgelatency);
  gzclose(new_edgeparid);
  gzclose(new_edgevarid);
#ifdef DDEBUG
  std::cerr << "=======End Write Graph With Isolated Edges=====" << std::endl;
#endif
  return new_edge_id;
}

void Datapath::readMethodGraph(MethodGraph &tmp_method_graph)
{
#ifdef DDEBUG
  std::cerr << "=======Read Graph=====" << std::endl;
#endif
  string gn(graphName);
  string graph_file_name(gn + "_method_graph");
  
  boost::dynamic_properties dp;
  boost::property_map<MethodGraph, boost::vertex_name_t>::type v_name = get(boost::vertex_name, tmp_method_graph);
  boost::property_map<MethodGraph, boost::edge_name_t>::type e_name = get(boost::edge_name, tmp_method_graph);
  dp.property("n_id", v_name);
  dp.property("e_inst", e_name);
  std::ifstream fin(graph_file_name.c_str());
  boost::read_graphviz(fin, tmp_method_graph, dp, "n_id");
  
#ifdef DDEBUG
  std::cerr << "=======End Read Graph=====" << std::endl;
#endif
}


void Datapath::readGraph(Graph &tmp_graph)
{
#ifdef DDEBUG
  std::cerr << "=======Read Graph=====" << std::endl;
#endif
  string gn(graphName);
  string graph_file_name(gn + "_graph");
  
  boost::dynamic_properties dp;
  boost::property_map<Graph, boost::vertex_name_t>::type v_name = get(boost::vertex_name, tmp_graph);
  boost::property_map<Graph, boost::edge_name_t>::type e_name = get(boost::edge_name, tmp_graph);
  dp.property("n_id", v_name);
  dp.property("e_id", e_name);
  std::ifstream fin(graph_file_name.c_str());
  boost::read_graphviz(fin, tmp_graph, dp, "n_id");

#ifdef DDEBUG
  std::cerr << "=======End Read Graph=====" << std::endl;
#endif
}

//initFunctions
void Datapath::writeRegStats()
{
  string bn(benchName);
  string tmp_name = bn + "_reg_stats";
  ofstream stats;
  stats.open(tmp_name.c_str());
  for (unsigned level_id = 0; ((int) level_id) < cycle; ++level_id)
    stats << regStats.at(level_id).size << "," << regStats.at(level_id).reads << "," << regStats.at(level_id).writes << endl; 
  stats.close();
  regStats.clear();
}
void Datapath::writePerCycleActivity()
{
  string bn(benchName);
  std::vector<int> mul_activity(cycle, 0);
  std::vector<int> add_activity(cycle, 0);
  std::unordered_map< string, std::vector<int> > ld_activity;
  std::unordered_map< string, std::vector<int> > st_activity;
  std::vector<string> partition_names;
  scratchpad->partitionNames(partition_names);
  for (auto it = partition_names.begin(); it != partition_names.end() ; ++it)
  {
    string p_name = *it;
    std::vector<int> tmp_activity(cycle, 0);
    ld_activity[p_name] = tmp_activity;
    st_activity[p_name] = tmp_activity;
  }
  //cerr <<  "start activity" << endl;
  for(unsigned node_id = 0; node_id < numTotalNodes; ++node_id)
  {
    if (globalIsolated.at(node_id))
      continue;
    int tmp_level = newLevel.at(node_id);
    if (microop.at(node_id) == IRMUL || microop.at(node_id) == IRMULOVF ||
        microop.at(node_id) == IRDIV)
      mul_activity.at(tmp_level) += 1;
    else if  (microop.at(node_id) == IRADD || microop.at(node_id) == IRADDOVF ||
      microop.at(node_id) == IRSUB || microop.at(node_id) == IRSUBOVF)
      add_activity.at(tmp_level) += 1;
    else if (is_load_op(microop.at(node_id)))
    {
      string base_addr = baseAddress.at(node_id);
      ld_activity[base_addr].at(tmp_level) += 1;
    }
    else if (is_store_op(microop.at(node_id)))
    {
      string base_addr = baseAddress.at(node_id);
      st_activity[base_addr].at(tmp_level) += 1;
    }
  }
  ofstream stats;
  string tmp_name = bn + "_stats";
  stats.open(tmp_name.c_str());
  stats << "cycles," << cycle << "," << numTotalNodes << endl; 
  stats << cycle << ",mul,add," ;
  for (auto it = partition_names.begin(); it != partition_names.end() ; ++it)
    stats << *it << "," ;
  stats << endl;
  for (unsigned tmp_level = 0; ((int)tmp_level) < cycle ; ++tmp_level)
  {
    stats << tmp_level << "," << mul_activity.at(tmp_level) << "," << add_activity.at(tmp_level) << ","; 
    for (auto it = partition_names.begin(); it != partition_names.end() ; ++it)
      stats << ld_activity.at(*it).at(tmp_level) << "," << st_activity.at(*it).at(tmp_level) << "," ;
    stats << endl;
  }
  stats.close();
}

void Datapath::writeGlobalIsolated()
{
  string file_name(benchName);
  file_name += "_global_isolated.gz";
  write_gzip_bool_file(file_name, globalIsolated.size(), globalIsolated);
}
void Datapath::writeFinalLevel()
{
  string file_name(benchName);
  file_name += "_level.gz";
  write_gzip_file(file_name, newLevel.size(), newLevel);
}
void Datapath::initMicroop(std::vector<int> &microop)
{
  string file_name(benchName);
  file_name += "_microop.gz";
  read_gzip_file(file_name, microop.size(), microop);
}
void Datapath::writeMicroop(std::vector<int> &microop)
{
  string file_name(benchName);
  file_name += "_microop.gz";
  write_gzip_file(file_name, microop.size(), microop);
}
void Datapath::initDynamicMethodID(std::vector<string> &methodid)
{
  string file_name(benchName);
  file_name += "_dynamic_methodid.gz";
  read_gzip_string_file(file_name, methodid.size(), methodid);
}
void Datapath::initMethodID(std::vector<int> &methodid)
{
  string file_name(benchName);
  file_name += "_methodid.gz";
  read_gzip_file(file_name, methodid.size(), methodid);
}
void Datapath::initInstID(std::vector<int> &instid)
{
  string file_name(benchName);
  file_name += "_instid.gz";
  read_gzip_file(file_name, instid.size(), instid);
}
void Datapath::initAddress(std::vector<unsigned> &address)
{
  string file_name(benchName);
  file_name += "_memaddr.gz";
  read_gzip_1in2_unsigned_file(file_name, address.size(), address);
}
void Datapath::initAddressAndSize(std::vector<pair<unsigned, unsigned> > &address)
{
  string file_name(benchName);
  file_name += "_memaddr.gz";
  read_gzip_2_unsigned_file(file_name, address.size(), address);
}

void Datapath::initializeGraphInMap(std::unordered_map<string, edgeAtt> &full_graph)
{
  Graph tmp_graph;
  readGraph(tmp_graph); 
  unsigned num_of_edges = boost::num_edges(tmp_graph);
  
  VertexNameMap vertex_to_name = get(boost::vertex_name, tmp_graph);
  EdgeNameMap edge_to_name = get(boost::edge_name, tmp_graph);
  
  std::vector<unsigned> edge_latency(num_of_edges, 0);
  std::vector<int> edge_parid(num_of_edges, 0);
  std::vector<int> edge_varid(num_of_edges, 0);

  initEdgeLatency(edge_latency);
  initEdgeParID(edge_parid);
  initEdgeVarID(edge_varid);
  
  //initialize full_graph
  edge_iter ei, ei_end;
  for (tie(ei, ei_end) = edges(tmp_graph); ei != ei_end; ++ei)
  {
    int edge_id = edge_to_name[*ei];
    int from = vertex_to_name[source(*ei, tmp_graph)];
    int to   = vertex_to_name[target(*ei, tmp_graph)];
    ostringstream oss;
    oss << from << "-" << to;
    full_graph[oss.str()] = {edge_varid.at(edge_id), edge_parid.at(edge_id), edge_latency.at(edge_id)};
  }
}

void Datapath::writeGraphInMap(std::unordered_map<string, edgeAtt> &full_graph, string name)
{
#ifdef DEBUG
  std::cerr << "=======Write Graph In Map: " << name << "," << full_graph.size() << "=====" << std::endl;
#endif
  ofstream graph_file;
  gzFile new_edgelatency, new_edgeparid, new_edgevarid;
  
  string graph_name;
  string edge_varid_file, edge_parid_file, edge_latency_file;

  edge_varid_file = name + "_edgevarid.gz";
  edge_parid_file = name + "_edgeparid.gz";
  edge_latency_file = name + "_edgelatency.gz";
  graph_name = name + "_graph";
  
  new_edgelatency = gzopen(edge_latency_file.c_str(), "w");
  new_edgeparid = gzopen(edge_parid_file.c_str(), "w");
  new_edgevarid = gzopen(edge_varid_file.c_str(), "w");
  
  graph_file.open(graph_name.c_str());
  graph_file << "digraph DDDG {" << std::endl;
  for (unsigned node_id = 0; node_id < numTotalNodes; node_id++)
     graph_file << node_id << ";" << std::endl; 
  int new_edge_id = 0; 
  for(auto it = full_graph.begin(); it != full_graph.end(); it++)
  {
    int from, to;
    sscanf(it->first.c_str(), "%d-%d", &from, &to);
    
    graph_file << from << " -> " 
              << to
              << " [e_id = " << new_edge_id << "];" << endl;
    new_edge_id++;
    gzprintf(new_edgelatency, "%d\n", it->second.latency);
    gzprintf(new_edgeparid, "%d\n", it->second.parid);
    gzprintf(new_edgevarid, "%d\n", it->second.varid);
  }
  graph_file << "}" << endl;
  graph_file.close();
  gzclose(new_edgelatency);
  gzclose(new_edgeparid);
  gzclose(new_edgevarid);
#ifdef DDEBUG
  //std::cerr << "=======End Write Graph In Map=====" << std::endl;
#endif
}

void Datapath::writeParType(std::vector<int> &partype, int id)
{
  ostringstream file_name;
  file_name << benchName << "_par" << id << "type.gz";
  write_gzip_file(file_name.str(), partype.size(), partype);
}
void Datapath::writeParVid(std::vector<int> &parvid, int id)
{
  ostringstream file_name;
  file_name << benchName << "_par" << id << "vid.gz";
  write_gzip_file(file_name.str(), parvid.size(), parvid);
}
void Datapath::writeParValue(std::vector<string> &parvalue, int id)
{
  ostringstream file_name;
  file_name << benchName << "_par" << id << "value.gz";
  write_gzip_string_file(file_name.str(), parvalue.size(), parvalue);
}
void Datapath::initParType(std::vector<int> &partype, int id)
{
  ostringstream file_name;
  file_name << benchName << "_par" << id << "type.gz";
  read_gzip_file(file_name.str(), partype.size(), partype);
}
void Datapath::initParVid(std::vector<int> &parvid, int id)
{
  ostringstream file_name;
  file_name << benchName << "_par" << id << "vid.gz";
  read_gzip_file(file_name.str(), parvid.size(), parvid);
}
void Datapath::initResultVid(std::vector<string> &parvid)
{
  ostringstream file_name;
  file_name << benchName << "_result_varid.gz";
  read_gzip_string_file(file_name.str(), parvid.size(), parvid);
}
void Datapath::initParValue(std::vector<string> &parvalue, int id)
{
  ostringstream file_name;
  file_name << benchName << "_par" << id << "value.gz";
  read_gzip_string_file(file_name.str(), parvalue.size(), parvalue);
}

void Datapath::initEdgeParID(std::vector<int> &parid)
{
  string file_name(graphName);
  file_name += "_edgeparid.gz";
  read_gzip_file(file_name, parid.size(), parid);
}
void Datapath::initEdgeVarID(std::vector<int> &varid)
{
  string file_name(graphName);
  file_name += "_edgevarid.gz";
  read_gzip_file(file_name, varid.size(), varid);
}
void Datapath::initEdgeLatency(std::vector<unsigned> &edge_latency)
{
  string file_name(graphName);
  file_name += "_edgelatency.gz";
  read_gzip_unsigned_file(file_name, edge_latency.size(), edge_latency);
}
void Datapath::writeEdgeLatency(std::vector<unsigned> &edge_latency)
{
  string file_name(graphName);
  file_name += "_edgelatency.gz";
  write_gzip_unsigned_file(file_name, edge_latency.size(), edge_latency);
}
void Datapath::initMemBaseInNumber(std::vector<unsigned> &base)
{
  string file_name(benchName);
  file_name += "_membase.gz";
  read_gzip_unsigned_file(file_name, base.size(), base);
}
void Datapath::initMemBaseInString(std::vector<string> &base)
{
  string file_name(benchName);
  file_name += "_membase.gz";
  read_gzip_string_file(file_name, base.size(), base);
}
void Datapath::writeMemBaseInNumber(std::vector<unsigned> &base)
{
  string file_name(benchName);
  file_name += "_membase.gz";
  write_gzip_unsigned_file(file_name, base.size(), base);
}



//stepFunctions
//multiple function, each function is a separate graph
void Datapath::setGraphForStepping(string graph_name)
{
  graphName = (char*)graph_name.c_str();
  string gn(graphName);
  
  string graph_file_name(gn + "_graph");

#ifdef DEBUG
  std::cerr << "========Setting Graph for Stepping======" << std::endl;
#endif
  boost::dynamic_properties dp;
  boost::property_map<Graph, boost::vertex_name_t>::type v_name = get(boost::vertex_name, graph_);
  boost::property_map<Graph, boost::edge_name_t>::type e_name = get(boost::edge_name, graph_);
  dp.property("n_id", v_name);
  dp.property("e_id", e_name);
  std::ifstream fin(graph_file_name.c_str());
  boost::read_graphviz(fin, graph_, dp, "n_id");
  
  numGraphEdges = boost::num_edges(graph_);
  numGraphNodes = boost::num_vertices(graph_);
  
  vertexToName = get(boost::vertex_name, graph_);
  BGL_FORALL_VERTICES(v, graph_, Graph)
    nameToVertex[get(boost::vertex_name, graph_, v)] = v;
  
  edgeLatency.assign(numGraphEdges, 0);
  read_gzip_file(gn + "_edgelatency.gz", numGraphEdges, edgeLatency);
  
  isolated.assign(numGraphNodes, 0);
  numParents.assign(numGraphNodes, 0);
  
  totalConnectedNodes = 0;
  vertex_iter vi, vi_end;
  for (tie(vi, vi_end) = vertices(graph_); vi != vi_end; ++vi)
  {
    if (boost::degree(*vi, graph_) == 0)
      isolated.at(vertexToName[*vi]) = 1;
    else
    {
      numParents.at(vertexToName[*vi]) = boost::in_degree(*vi, graph_);
      totalConnectedNodes++;
    }
  }
  
  updateGlobalIsolated();

#ifdef DEBUG
  cerr << "totalConnectedNodes," << totalConnectedNodes << endl;
#endif

  executedNodes = 0;

  prevCycle = cycle;

  memReadyQueue.clear();
  nonMemReadyQueue.clear();
  executedQueue.clear();
  std::set<unsigned>().swap(memReadyQueue);
  std::set<unsigned>().swap(nonMemReadyQueue);
  std::vector<pair<unsigned,float> >().swap(executedQueue);

  initReadyQueue();
#ifdef DDEBUG
  cerr << "End of Setting Graph: " << graph_name << endl;
#endif
}

int Datapath::clearGraph()
{
  string gn(graphName);

#ifdef DEBUG
  cerr << gn << "," << cycle-prevCycle << endl;
#endif
  
  std::vector< Vertex > topo_nodes;
  boost::topological_sort(graph_, std::back_inserter(topo_nodes));
  //bottom nodes first
  std::vector<int> earliest_child(numGraphNodes, cycle);
  for (auto vi = topo_nodes.begin(); vi != topo_nodes.end(); ++vi)
  {
    int node_id = vertexToName[*vi];
    if (isolated.at(node_id))
      continue;
    if (!is_memory_op(microop.at(node_id)))
      if ((earliest_child.at(node_id) - 1 ) > newLevel.at(node_id))
        newLevel.at(node_id) = earliest_child.at(node_id) - 1;
    
    in_edge_iter in_i, in_end;
    for (tie(in_i, in_end) = in_edges(*vi , graph_); in_i != in_end; ++in_i)
    {
      int parent_id = vertexToName[source(*in_i, graph_)];
      if (earliest_child.at(parent_id) > newLevel.at(node_id))
        earliest_child.at(parent_id) = newLevel.at(node_id);
    }
  }
  earliest_child.clear();
  updateRegStats();
  edgeLatency.clear();
  //callLatency.clear();
  numParents.clear();
  isolated.clear();
  nameToVertex.clear();
  //vertexToName.clear();
  std::vector<int>().swap(edgeLatency);
  std::vector<bool>().swap(isolated);
  std::vector<int>().swap(numParents);
  
  graph_.clear();
  return cycle-prevCycle;
}
void Datapath::updateRegStats()
{
  std::map<int, Vertex> name_to_vertex;
  BGL_FORALL_VERTICES(v, graph_, Graph)
    name_to_vertex[get(boost::vertex_name, graph_, v)] = v;
  
  for(unsigned node_id = 0; node_id < numGraphNodes; node_id++)
  {
    if (isolated.at(node_id))
      continue;
    if (is_branch_inst(microop.at(node_id)) || 
        is_index_inst(microop.at(node_id)))
      continue;
    int node_level = newLevel.at(node_id);
    int max_children_level 		= node_level;
    
    Vertex node = name_to_vertex[node_id];
    out_edge_iter out_edge_it, out_edge_end;
    std::set<int> children_levels;
    for (tie(out_edge_it, out_edge_end) = out_edges(node, graph_); out_edge_it != out_edge_end; ++out_edge_it)
    {
      int child_id = vertexToName[target(*out_edge_it, graph_)];
      //if (!is_memory_op(microop.at(child_id)))
        //continue;
      
      if (is_memory_op(microop.at(child_id)) && 
          (!is_store_op(microop.at(node_id))))
        continue;
      //if (is_branch_inst(microop.at(child_id)) ||
      //    is_index_inst(microop.at(child_id)) )
      //  continue;
      
      int child_level = newLevel.at(child_id);
      if (child_level > max_children_level)
        max_children_level = child_level;
      if (child_level > node_level  && child_level != cycle - 1)
        children_levels.insert(child_level);
        
    }
    for (auto it = children_levels.begin(); it != children_levels.end(); it++)
        regStats.at(*it).reads++;
    
    if (max_children_level > node_level )
      regStats.at(node_level).writes++;
    //for (int i = node_level; i < max_children_level; ++i)
      //regStats.at(i).size++;
  }
}
bool Datapath::step()
{
//#ifdef DEBUG
  //cerr << "===========Stepping============" << endl;
//#endif
  int firedNodes = fireNonMemNodes();
  firedNodes += fireMemNodes();

  stepExecutedQueue();

#ifdef DDEBUG
  cerr << "Cycle:" << cycle << ",executedNodes," << executedNodes << ",totalConnectedNodes," << totalConnectedNodes << endl;
#endif
  cycle++;
  if (executedNodes == totalConnectedNodes)
    return 1;
  return 0;
}

void Datapath::stepExecutedQueue()
{
#ifdef DDEBUG
  cerr << "======stepping executed queue " << endl;
#endif
  
  auto it = executedQueue.begin();
  while (it != executedQueue.end())
  {
    //it->second is the number of cycles to execute current nodes
#ifdef DEBUG
    cerr << "executing," << it->first << "," << microop.at(it->first) << "," <<
    cycle<< endl;
#endif
    if (it->second <= cycleTime)
    {
      unsigned node_id = it->first;
      executedNodes++;
      newLevel.at(node_id) = cycle;
      it = executedQueue.erase(it);
    }
    else
    {
      it->second -= cycleTime;
      it++;
    }
  }
#ifdef DEBUG
  cerr << "======End stepping executed queue " << endl;
#endif
}

void Datapath::updateChildren(unsigned node_id, float latencySoFar)
{
#ifdef DDEBUG
  cerr << "updating the children of " << node_id << endl;
#endif
  Vertex node = nameToVertex[node_id];
  out_edge_iter out_edge_it, out_edge_end;
  for (tie(out_edge_it, out_edge_end) = out_edges(node, graph_); out_edge_it != out_edge_end; ++out_edge_it)
  {
    int child_id = vertexToName[target(*out_edge_it, graph_)];
#ifdef DDEBUG
    cerr << "child_id, numParents: " << child_id << "," << numParents[child_id] << endl;
#endif
    if (numParents[child_id] > 0)
    {
      numParents[child_id]--;
      if (numParents[child_id] == 0)
      {
        if (is_memory_op(microop.at(child_id)))
          addMemReadyNode(child_id);
        else
        {
          executedQueue.push_back(make_pair(child_id, latencySoFar + node_latency(microop.at(child_id))));
          updateChildren(child_id, latencySoFar + node_latency(microop.at(child_id)));
        }
      }
    }
  }
}
int Datapath::fireMemNodes()
{
  int firedNodes = 0;
  auto it = memReadyQueue.begin();
  while (it != memReadyQueue.end() && scratchpad->canService())
  {
    unsigned node_id = *it;
    //need to check scratchpad constraints
    string node_part = baseAddress.at(node_id);
    if(scratchpad->canServicePartition(node_part))
    {
      float latency = scratchpad->addressRequest(node_part);
      assert(latency != -1);
      //assign levels and add to executed queue
      executedQueue.push_back(make_pair(node_id, node_latency(microop.at(node_id))));
      updateChildren(node_id, node_latency(microop.at(node_id)));
      it = memReadyQueue.erase(it);
      firedNodes++;
    }
    else
      ++it;
  }
#ifdef DEBUG
  cerr << "fired Memory Nodes," << firedNodes << endl;
#endif
  return firedNodes;
}

int Datapath::fireNonMemNodes()
{
#ifdef DEBUG
  cerr << "=========Firing NonMemory Nodes========" << endl;
#endif
  int firedNodes = 0;
  //assume the Queue is sorted by somehow
  //non considering user's constraints on num of functional units
  auto it = nonMemReadyQueue.begin();
  while (it != nonMemReadyQueue.end())
  {
    unsigned node_id = *it;
    firedNodes++;
    executedQueue.push_back(make_pair(node_id, node_latency(microop.at(node_id))));
    updateChildren(node_id, node_latency(microop.at(node_id)));
    it = nonMemReadyQueue.erase(it);
  }
#ifdef DEBUG
  cerr << "Fired Non-Memory Nodes: " << firedNodes << endl;
#endif
  return firedNodes;
}

void Datapath::initReadyQueue()
{
#ifdef DEBUG
  cerr << "======Initializing Ready Queue=========" << endl;
#endif
  for(unsigned i = 0; i < numGraphNodes; i++)
  {
    if (numParents[i] == 0 && isolated[i] != 1)
    {
      if (is_memory_op(microop.at(i)))
        addMemReadyNode(i);
      else
        addNonMemReadyNode(i);
    }
  }
#ifdef DEBUG
  cerr << "initialreadyqueuesize: memory," << memReadyQueue.size() << ", non-mem," << nonMemReadyQueue.size() << endl;
#endif
}

void Datapath::addMemReadyNode(unsigned node_id)
{
  memReadyQueue.insert(node_id);
}

void Datapath::addNonMemReadyNode(unsigned node_id)
{
  nonMemReadyQueue.insert(node_id);
}

void Datapath::updateGlobalIsolated()
{
  for (unsigned node_id = 0; node_id < numGraphNodes; ++node_id)
  {
    if (isolated.at(node_id) == 0)
      globalIsolated.at(node_id) = 0;
  }
}

//readConfigs
bool Datapath::readUnrollingConfig(std::unordered_map<string, pair<int, int> > &unrolling_config)
{
  ifstream config_file;
  string file_name(benchName);
  file_name += "_unrolling_config";
  config_file.open(file_name.c_str());
  if (!config_file.is_open())
  {
    //FIXME: if undefined, should assume no partition
    std::cerr << "unrolling config file undefined" << endl;
    exit(0);
  }
  while(!config_file.eof())
  {
    string wholeline;
    getline(config_file, wholeline);
    if (wholeline.size() == 0)
      break;
    int methodid, instid, factor;
    sscanf(wholeline.c_str(), "%d,%d,%d\n", &methodid, &instid, &factor);
    ostringstream oss;
    oss << methodid << "-" << instid;
    unrolling_config[oss.str()] = make_pair(factor, 0);
    //cerr << oss.str() << "," << factor << endl;
  }
  config_file.close();
  return 1;
}

bool Datapath::readFlattenConfig(std::unordered_set<string> &flatten_config)
{
  ifstream config_file;
  string file_name(benchName);
  file_name += "_flatten_config";
  config_file.open(file_name.c_str());
  if (!config_file.is_open())
    return 0;
  while(!config_file.eof())
  {
    string wholeline;
    getline(config_file, wholeline);
    if (wholeline.size() == 0)
      break;
    int methodid, instid;
    sscanf(wholeline.c_str(), "%d,%d\n", &methodid, &instid);
    ostringstream oss;
    oss << methodid << "-" << instid;
    flatten_config.insert(oss.str());
  }
  config_file.close();
  return 1;
}


bool Datapath::readInductionConfig(std::unordered_set<string> &ind_config)
{
  ifstream config_file;
  string file_name(benchName);
  file_name += "_induction_variables";
  config_file.open(file_name.c_str());
  if (!config_file.is_open())
    return 0;
  while(!config_file.eof())
  {
    string wholeline;
    getline(config_file, wholeline);
    if (wholeline.size() == 0)
      break;
    char fun[256];
    int methodid, varid, baseid, instid, loopid, p_loopid;
    sscanf(wholeline.c_str(), "%[^,],%d,%d,%d,%d,%d,%d\n", fun,&methodid, &varid, &baseid, &instid, &loopid, &p_loopid);
    
    ostringstream oss;
    oss << methodid << "-" << varid ;
    //cerr << "ind_config inserting: " << oss.str() << endl;
    ind_config.insert(oss.str());
  }
  config_file.close();
  return 1;
}
void Datapath::readCompletePartitionConfig(std::unordered_set<unsigned> &config)
{
  string bn(benchName);
  string comp_partition_file;
  comp_partition_file = bn + "_complete_partition_config";
  
  ifstream config_file;
  config_file.open(comp_partition_file);
  string wholeline;
  while(!config_file.eof())
  {
    getline(config_file, wholeline);
    if (wholeline.size() == 0)
      break;
    size_t pos = wholeline.find(',');
    unsigned addr = atoi(wholeline.substr(0,pos).c_str());
    config.insert(addr);
  }
  config_file.close();
}

void Datapath::readPartitionConfig(std::unordered_map<unsigned, partitionEntry> & partition_config)
{
  ifstream config_file;
  string file_name(benchName);
  file_name += "_partition_config";
  config_file.open(file_name.c_str());
  string wholeline;
  if (config_file.is_open())
  {
    while (!config_file.eof())
    {
      
      getline(config_file, wholeline);
      if (wholeline.size() == 0) break;
      
      unsigned base_addr, size, p_factor;
      char type[256];
      sscanf(wholeline.c_str(), "%d,%[^,],%d,%d,\n", &base_addr, type, &size, &p_factor);
      string p_type(type);
      partition_config[base_addr] = {p_type, size, p_factor};
    }
    config_file.close();
  }
  else
  {
    //FIXME: if undefined, should assume no partition
    std::cerr << "partition config file undefined" << endl;
    exit(0);
  }
}
