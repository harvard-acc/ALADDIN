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
  baseAddress.assign(numTotalNodes, "");
  cycle = 0;
  g = new igraph_t;
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
}

void Datapath::globalOptimizationPass()
{
#ifdef DEBUG
  std::cerr << "=======Global Optimization Pass=====" << std::endl;
#endif
  //graph independent
  ////complete partition
  completePartition();
  //partition
  scratchpadPartition();
  //node strength reduction
  ////remove induction variables
  removeInductionDependence();
  ////remove address calculation
  removeAddressCalculation();
  ////remove branch edges
  removeBranchEdges();
  //have to happen after address calculation:FIXME
  nodeStrengthReduction();

  addCallDependence();

  methodGraphBuilder();
  methodGraphSplitter();
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
  igraph_t *tmp_g;
  tmp_g = new igraph_t;
  readGraph(tmp_g); 
  unsigned num_edges = (unsigned) igraph_ecount(tmp_g);
#ifdef DEBUG
  std::cerr << "num_edges," << num_edges << std::endl;
#endif
  
  std::vector<int> edge_parid (num_edges, 0);
  initEdgeParID(edge_parid);
  //read edgetype.second
  std::vector<bool> to_remove_edges(num_edges, 0);
  for(unsigned node_id = 0; node_id < numTotalNodes; ++node_id)
  {
    int node_microop = microop.at(node_id);
    if (!is_memory_op(node_microop))
      continue;
    unsigned base_addr = orig_base.at(node_id);
    if (base_addr == 0)
      continue;
    if (comp_part_config.find(base_addr) == comp_part_config.end())
      continue;
    changed_nodes++;
    
    igraph_vs_t vs_parents;
    igraph_vit_t vit_parents;
    igraph_vs_adj(&vs_parents, node_id, IGRAPH_IN);
    igraph_vit_create(tmp_g, vs_parents, &vit_parents);
    while (!IGRAPH_VIT_END(vit_parents))
    {
      unsigned parent_id = (unsigned)IGRAPH_VIT_GET(vit_parents);
      //find the address calculation chain
      unsigned edge_id = check_edgeid(parent_id, node_id, tmp_g);
      int parid = edge_parid.at(edge_id);
      if (parid ==1 )
        to_remove_edges.at(edge_id) = 1;

      IGRAPH_VIT_NEXT(vit_parents);
    }
    igraph_vs_destroy(&vs_parents);
    igraph_vit_destroy(&vit_parents);
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
  igraph_t *tmp_g;
  tmp_g = new igraph_t;
  readGraph(tmp_g); 
  unsigned num_edges = (unsigned) igraph_ecount(tmp_g);
#ifdef DEBUG
  std::cerr << "num_edges," << num_edges << std::endl;
#endif
  /*track the number of children each node has*/
  vector<int> num_of_children(numTotalNodes, 0);
  initialized_num_of_children(tmp_g, num_of_children);

  unordered_set<unsigned> to_remove_nodes;
  igraph_vector_t v_topological;
  igraph_vector_init(&v_topological, numTotalNodes);
  igraph_topological_sorting(tmp_g, &v_topological, IGRAPH_IN);
  for (unsigned i = 0; i < numTotalNodes; ++i)
  {
    unsigned node_id = (unsigned)VECTOR(v_topological)[i];
    assert(num_of_children.at(node_id) >= 0);

    if (num_of_children.at(node_id) == 0 
      && microop.at(node_id) != IRSTOREREL
      && microop.at(node_id) != IRRET 
      && microop.at(node_id) != IRBRANCHIF 
      && microop.at(node_id) != IRBRANCHIFNOT
      && microop.at(node_id) != IRCALL)
    {
      to_remove_nodes.insert(node_id); 
      
      igraph_vs_t vs_parent;
      igraph_vit_t vit_parent_it;
      igraph_vs_adj(&vs_parent, node_id, IGRAPH_IN);
      igraph_vit_create(tmp_g, vs_parent, &vit_parent_it);

      while(!IGRAPH_VIT_END(vit_parent_it))
      {
        int parent_id = (int)IGRAPH_VIT_GET(vit_parent_it);
        assert(num_of_children.at(parent_id) != 0);
        num_of_children.at(parent_id)--;
        IGRAPH_VIT_NEXT(vit_parent_it);
      }
      igraph_vs_destroy(&vs_parent);
      igraph_vit_destroy(&vit_parent_it);
    }
  }
  igraph_vector_destroy(&v_topological);
  writeGraphWithIsolatedNodes(to_remove_nodes);
#ifdef DEBUG
  std::cerr << "=======Cleaned Leaf Nodes:" << to_remove_nodes.size() << "=====" << std::endl;
#endif

}

void Datapath::removeInductionDependence()
{
#ifdef DEBUG
  std::cerr << "=======Remove Induction Dependence=====" << std::endl;
#endif
  //set graph
  igraph_t *tmp_g;
  tmp_g = new igraph_t;
  readGraph(tmp_g); 
  unsigned num_edges = (unsigned) igraph_ecount(tmp_g);
#ifdef DEBUG
  std::cerr << "num_edges," << num_edges << std::endl;
#endif
  std::vector<int> par4vid(numTotalNodes, 0);
  initParVid(par4vid, 4);

  std::vector<int> methodid(numTotalNodes, 0);
  initMethodID(methodid);
  
  std::vector<unsigned> edge_latency(num_edges, 0);
  initEdgeLatency(edge_latency);

  std::unordered_set<string> induction_config;
  readInductionConfig(induction_config);
  
  igraph_vector_t v_topological;
  igraph_vector_init(&v_topological, numTotalNodes);
  igraph_topological_sorting(tmp_g, &v_topological, IGRAPH_OUT);

  int removed_edges = 0;
  for (unsigned i = 0; i < numTotalNodes; ++i)
  {
    unsigned node_id = (unsigned)VECTOR(v_topological)[i];
    int node_resultid = par4vid.at(node_id);
    int node_methodid = methodid.at(node_id);
    ostringstream combined_id;
    combined_id << node_methodid << "-" << node_resultid;
    if (induction_config.find(combined_id.str()) == induction_config.end())
      continue;
    
    igraph_vs_t vs_child;
    igraph_vit_t vit_child_it;
    igraph_vs_adj(&vs_child, node_id, IGRAPH_OUT);
    igraph_vit_create(tmp_g, vs_child, &vit_child_it);
    while(!IGRAPH_VIT_END(vit_child_it))
    {
      unsigned child_node = (unsigned) IGRAPH_VIT_GET(vit_child_it);
      
      int edge_id = check_edgeid(node_id, child_node, tmp_g);
      edge_latency.at(edge_id) = 0;
      removed_edges++;
      IGRAPH_VIT_NEXT(vit_child_it);
    }
    igraph_vs_destroy(&vs_child);
    igraph_vit_destroy(&vit_child_it);
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
  igraph_t *tmp_g;
  tmp_g = new igraph_t;
  readGraph(tmp_g); 
  unsigned num_edges = (unsigned) igraph_ecount(tmp_g);
#ifdef DEBUG
  std::cerr << "num_edges," << num_edges << std::endl;
#endif
  std::vector<bool> to_remove_edges(num_edges, 0);
  std::vector<newEdge> to_add_edges;

  std::vector<int> edge_parid(num_edges, 0);
  initEdgeParID(edge_parid);

  std::vector<int> par2vid(numTotalNodes, 0);
  initParVid(par2vid, 2);

  std::vector<bool> updated(numTotalNodes, 0);
  
  int address_cal_edges = 0;

  igraph_vector_t v_topological;
  igraph_vector_init(&v_topological, numTotalNodes);
  igraph_topological_sorting(tmp_g, &v_topological, IGRAPH_IN);
  for (unsigned i = 0; i < numTotalNodes; ++i)
  {
    unsigned node_id = (unsigned)VECTOR(v_topological)[i];
    if (updated.at(node_id))
      continue;
    updated.at(node_id) = 1;
    int node_microop = microop.at(node_id);
    if (node_microop != IRLOADREL && node_microop != IRSTOREREL)
      continue;
    
    igraph_vs_t vs_parent;
    igraph_vit_t vit_parent_it;
    igraph_vs_adj(&vs_parent, node_id, IGRAPH_IN);	
    igraph_vit_create(tmp_g, vs_parent, &vit_parent_it);

    while(!IGRAPH_VIT_END(vit_parent_it))
    {
      unsigned parent_id	= (unsigned) IGRAPH_VIT_GET(vit_parent_it);
      int parent_microop	= microop.at(parent_id);
      if (parent_microop == IRADD)
      {
        int edge_id = check_edgeid(parent_id, node_id, tmp_g);
        int load_add_edge_parid = edge_parid.at(edge_id);
        if(load_add_edge_parid == 1)
        {
          address_cal_edges++;
          //remove the edge between the laod and the add
          to_remove_edges.at(edge_id) = 1;
          
          igraph_vs_t vs_grandparent;
          igraph_vit_t vit_grandparent_it;
          igraph_vs_adj(&vs_grandparent, parent_id, IGRAPH_IN);
          igraph_vit_create(tmp_g, vs_grandparent, &vit_grandparent_it);
          while (!IGRAPH_VIT_END(vit_grandparent_it))
          {
            unsigned grandparent_id = (unsigned) IGRAPH_VIT_GET(vit_grandparent_it);
            int grandparent_microop = microop.at(grandparent_id);
            int p_edge_id = check_edgeid(grandparent_id, parent_id, tmp_g);
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
                igraph_vs_t vs_mul_parent;
                igraph_vit_t vit_mul_parent_it;
                igraph_vs_adj(&vs_mul_parent, grandparent_id, IGRAPH_IN);	
                igraph_vit_create(tmp_g, vs_mul_parent, &vit_mul_parent_it);
                int num_parent = IGRAPH_VIT_SIZE(vit_mul_parent_it);
                assert(num_parent == 1 || num_parent == 0);
                if (num_parent == 1)
                {
                  unsigned mul_parent = (unsigned) IGRAPH_VIT_GET(vit_mul_parent_it);
                  int m_edge_id = check_edgeid(mul_parent, grandparent_id, tmp_g);
                  address_cal_edges++;
                  //remove the edge between the mul and its parent
                  to_remove_edges.at(m_edge_id) = 1;
                  //add an edge between mul's parent and the memory node
                  newEdge *p = new newEdge[1];
                  p->from = mul_parent;
                  p->to = node_id;
                  p->varid = -1;
                  p->parid = load_add_edge_parid;
                  p->latency = 1;
                  to_add_edges.push_back(*p);
                }
                igraph_vs_destroy(&vs_mul_parent);
                igraph_vit_destroy(&vit_mul_parent_it);
                updated.at(grandparent_id) = 1;
              }
            }
            IGRAPH_VIT_NEXT(vit_grandparent_it);
          }
          igraph_vs_destroy(&vs_grandparent);
          igraph_vit_destroy(&vit_grandparent_it);
          updated.at(parent_id) = 1;
        }
      }
      IGRAPH_VIT_NEXT(vit_parent_it);
    }
    igraph_vs_destroy(&vs_parent);
    igraph_vit_destroy(&vit_parent_it);
  }
  writeGraphWithIsolatedEdges(to_remove_edges);
  writeGraphWithNewEdges(to_add_edges);

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
  igraph_t *tmp_g;
  tmp_g = new igraph_t;
  readGraph(tmp_g); 
  unsigned num_edges = (unsigned) igraph_ecount(tmp_g);
#ifdef DEBUG
  std::cerr << "num_edges," << num_edges << std::endl;
#endif
  std::vector<unsigned> edge_latency(num_edges, 0);
  initEdgeLatency(edge_latency);
  
  int branch_edges = 0;
  std::vector<bool> to_remove_edges(num_edges, 0);
  
  igraph_es_t es_edge;
  igraph_eit_t eit_edge_it;
  igraph_es_all(&es_edge, IGRAPH_EDGEORDER_ID);
  igraph_eit_create(tmp_g, es_edge, &eit_edge_it);
  IGRAPH_EIT_RESET(eit_edge_it);
	while(!IGRAPH_EIT_END(eit_edge_it))
  {
    igraph_integer_t edge_id = IGRAPH_EIT_GET(eit_edge_it);
    igraph_integer_t from, to;
    igraph_edge(tmp_g, edge_id, &from, &to);

    int from_microop = microop.at((int)from);
    int to_microop = microop.at((int)to);
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
    IGRAPH_EIT_NEXT(eit_edge_it);
  }
  igraph_es_destroy(&es_edge);
  igraph_eit_destroy(&eit_edge_it);
  
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
  std::unordered_map<string, unsigned> node_id_map;
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
    
    auto it = node_id_map.find(caller_method);
    if (it == node_id_map.end())
    {
      unsigned size = node_id_map.size();
      node_id_map[caller_method] = size;
      node_att.push_back(caller_method);
    }
    it = node_id_map.find(callee_method);
    if (it == node_id_map.end())
    {
      unsigned size = node_id_map.size();
      node_id_map[callee_method] = size;
      node_att.push_back(callee_method);
    }
    callDep *p = new callDep[1];
    p->caller = caller_method;
    p->callee = callee_method;
    p->callInstID = dynamic_id;
    method_dep.push_back(*p);
  }
  call_file.close();
  //write output
  ofstream edgelist_file, edge_att_file, node_att_file;

  string graph_file(benchName);
  string edge_file(benchName);
  string node_file(benchName);
  
  graph_file += "_method_graph";
  edge_file += "_method_graph_edge";
  node_file += "_method_graph_node";
  edgelist_file.open(graph_file);
  edge_att_file.open(edge_file);
  node_att_file.open(node_file);

  for (auto it = method_dep.begin(); it != method_dep.end(); ++it)
  {
    edgelist_file << node_id_map[it->caller] << " " << node_id_map[it->callee] <<endl;
    edge_att_file << it->callInstID << endl;
  }

  for(unsigned i = 0; i < node_att.size() ; i++)
    node_att_file << node_att.at(i) << endl;
  edgelist_file.close();
  node_att_file.close();
  edge_att_file.close();

}
void Datapath::methodGraphSplitter()
{
#ifdef DEBUG
  std::cerr << "=======Method Graph Splitter=====" << std::endl;
#endif
  //set graph
  igraph_t *tmp_method_g;
  tmp_method_g = new igraph_t;
  readMethodGraph(tmp_method_g); 

  unsigned num_method_nodes = (unsigned) igraph_vcount(tmp_method_g);
  
  string bn(benchName);
  std::vector<int> method_edge;
  string edge_file(bn);
  read_file(edge_file + "_method_graph_edge", method_edge);

  std::vector<string> method_node;
  string node_file(bn);
  read_string_file(node_file + "_method_graph_node", method_node);
  
  std::vector<string> dynamic_methodid(numTotalNodes, "");
  initDynamicMethodID(dynamic_methodid);

  std::vector<bool> updated(numTotalNodes, 0);
  
  igraph_vector_t v_topological;
  igraph_vector_init(&v_topological, num_method_nodes);
  igraph_topological_sorting(tmp_method_g, &v_topological, IGRAPH_IN);
  
  ofstream method_order;
  method_order.open(bn + "_method_order");

  for(unsigned method_i = 0; method_i < num_method_nodes; method_i++)
  {
    unsigned method_node_id = (unsigned) VECTOR(v_topological)[method_i];
    string method_name = method_node.at(method_node_id);
    cerr << "current method: " << method_name << endl;
    
    //find its ony parent
    igraph_vs_t vs_method_parent;
    igraph_vit_t vit_method_parent;
    igraph_vs_adj(&vs_method_parent, method_node_id, IGRAPH_IN);
    igraph_vit_create(tmp_method_g, vs_method_parent, &vit_method_parent);
    if (IGRAPH_VIT_SIZE(vit_method_parent) == 0)
      break;
    assert(IGRAPH_VIT_SIZE(vit_method_parent) == 1);

    unsigned parent_method = (unsigned) IGRAPH_VIT_GET(vit_method_parent);
    
    int method_edgeid = check_edgeid(parent_method, method_node_id, tmp_method_g);
    int call_inst = method_edge.at(method_edgeid);

    cerr << "call_inst," << call_inst << endl;
    method_order << method_name << "," <<
      method_node.at(parent_method) << "," << call_inst << endl;
    
    igraph_vs_destroy(&vs_method_parent);
    igraph_vit_destroy(&vit_method_parent);
    
    //set graph
    igraph_t *tmp_g;
    tmp_g = new igraph_t;
    readGraph(tmp_g); 
    unsigned num_edges = (unsigned) igraph_ecount(tmp_g);
    
    std::vector<unsigned> edge_latency(num_edges, 0);
    std::vector<int> edge_parid(num_edges, 0);
    std::vector<int> edge_varid(num_edges, 0);

    initEdgeLatency(edge_latency);
    initEdgeParID(edge_parid);
    initEdgeVarID(edge_varid);
  
    unordered_map<int, bool> to_split_nodes;
  
    unsigned node_id = call_inst + 1;
    while (node_id < numTotalNodes)
    {
      string node_dynamic_methodid = dynamic_methodid.at(node_id);
      if (updated[node_id])
      {
        ++node_id;
        continue;
      }
      if (node_dynamic_methodid.compare(method_name) != 0 )
        break;
      to_split_nodes[node_id] = 1;
      updated[node_id] = 1;
      ++node_id;
    }
    cerr << "ended node_id," << node_id << endl;
    
    //write output files
    ofstream new_graph_file;
    ogzstream new_edgeparid_file, new_edgevarid_file, new_edgelatency_file;
    
    string bn(benchName);
    new_graph_file.open(bn + "_graph");
    string file_name(bn);
    
    file_name += "_edgevarid.gz";
    new_edgevarid_file.open(file_name.c_str());
    
    file_name = bn + "_edgeparid.gz";
    new_edgeparid_file.open(file_name.c_str());
    
    file_name = bn + "_edgelatency.gz";
    new_edgelatency_file.open(file_name.c_str());
    
    unordered_map<string, bool> new_graph;
    unordered_map<string, bool> current_graph;

    ofstream current_graph_file;
    ogzstream current_edgeparid_file, current_edgevarid_file, current_edgelatency_file;
    
    string current_method_graph();
    string current_method_edgelatency();
    string current_method_edgetype(bn + "_" + method_name + "_edgetype.gz");
    
    current_graph_file.open(bn + "_" + method_name + "_graph");
    file_name = bn + "_" + method_name + "_edgevarid.gz";
    current_edgevarid_file.open(file_name.c_str());
    file_name = bn + "_" + method_name + "_edgeparid.gz";
    current_edgeparid_file.open(file_name.c_str());
    file_name = bn + "_" + method_name + "_edgelatency.gz";
    current_edgelatency_file.open(file_name.c_str());
    
    igraph_es_t es_edge;
    igraph_eit_t eit_edge_it;
    igraph_es_all(&es_edge, IGRAPH_EDGEORDER_ID);
    igraph_eit_create(tmp_g, es_edge, &eit_edge_it);
    IGRAPH_EIT_RESET(eit_edge_it);
    
    while(!IGRAPH_EIT_END(eit_edge_it))
    {
      igraph_integer_t edge_id = IGRAPH_EIT_GET(eit_edge_it);
      IGRAPH_EIT_NEXT(eit_edge_it); 
      
      igraph_integer_t from, to;
      igraph_edge(tmp_g, edge_id, &from, &to);
      
      if ((int)from == (int)to)
        continue;
      //cerr << "from,to," << from << "," << to << endl;
      auto split_from_it = to_split_nodes.find((int)from);
      auto split_to_it = to_split_nodes.find((int)to);
      if (split_from_it == to_split_nodes.end() 
          && split_to_it == to_split_nodes.end())
      {
        ostringstream oss;
        oss << (int) from << "-" << (int)to;
        if (new_graph.find(oss.str()) != new_graph.end())
          continue;
        new_graph[oss.str()] = 1;
        new_graph_file << (int)from << " " << (int)to << endl;
        new_edgeparid_file << edge_parid.at(edge_id) << endl; 
        new_edgevarid_file << edge_varid.at(edge_id) << endl; 
        new_edgelatency_file << edge_latency.at(edge_id) << endl;
      }
      
      else if (split_from_it == to_split_nodes.end() 
          && split_to_it != to_split_nodes.end())
      {
        ostringstream oss;
        oss << (int) from << "-" << (int)call_inst;
        if (new_graph.find(oss.str()) != new_graph.end())
          continue;
        new_graph[oss.str()] = 1;
        new_graph_file << (int)from << " " << call_inst << endl;
        new_edgeparid_file << edge_parid.at(edge_id) << endl; 
        new_edgevarid_file << edge_varid.at(edge_id) << endl; 
        new_edgelatency_file << edge_latency.at(edge_id) << endl;
      }
      else if (split_from_it != to_split_nodes.end() 
          && split_to_it == to_split_nodes.end())
      {
        ostringstream oss;
        oss << (int) call_inst << "-" << (int)to;
        if (new_graph.find(oss.str()) != new_graph.end())
          continue;
        new_graph[oss.str()] = 1;
        new_graph_file << call_inst << " " << (int) to << endl;
        new_edgeparid_file << edge_parid.at(edge_id) << endl; 
        new_edgevarid_file << edge_varid.at(edge_id) << endl; 
        new_edgelatency_file << edge_latency.at(edge_id) << endl;
      }
      else
      {
        ostringstream oss;
        oss << (int) from << "-" << (int)to;
        if (current_graph.find(oss.str()) != current_graph.end())
          continue;
        current_graph[oss.str()] = 1;
        
        current_graph_file << (int)from << " " << (int)to << endl;
        current_edgeparid_file << edge_parid.at(edge_id) << endl; 
        current_edgevarid_file << edge_varid.at(edge_id) << endl; 
        current_edgelatency_file << edge_latency.at(edge_id) << endl;
      }
    }
    igraph_es_destroy(&es_edge);
    igraph_eit_destroy(&eit_edge_it);

    new_graph_file.close();
    new_edgeparid_file.close();
    new_edgevarid_file.close();
    new_edgelatency_file.close();
    
    current_graph_file.close();
    current_edgeparid_file.close();
    current_edgevarid_file.close();
    current_edgelatency_file.close();
    igraph_destroy(tmp_g);
  }
  method_order.close();
	igraph_vector_destroy(&v_topological);
  igraph_destroy(tmp_method_g);
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
  igraph_t *tmp_g;
  tmp_g = new igraph_t;
  readGraph(tmp_g); 
  unsigned num_edges = (unsigned) igraph_ecount(tmp_g);
#ifdef DEBUG
  std::cerr << "num_edges," << num_edges << std::endl;
#endif
  std::vector<newEdge> to_add_edges;
  int last_call = -1;
  for(unsigned node_id = 0; node_id < numTotalNodes; node_id++)
  {
    if (microop.at(node_id) == IRCALL)
    {
      if (last_call != -1)
      {
        newEdge *p = new newEdge[1];
        p->from = last_call;
        p->to = node_id;
        p->varid = -1;
        p->parid = -1;
        p->latency = 1;
        to_add_edges.push_back(*p);
      }
      last_call = node_id;
    }
  }
  writeGraphWithNewEdges(to_add_edges);

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
  initAddressAndSize(address);

  std::vector<unsigned> orig_base(numTotalNodes, 0);
  read_gzip_unsigned_file(bn + "_membase.gz", numTotalNodes, orig_base);
  
  //read the partition config file to get the address range
  // <base addr, <type, part_factor> > 
  std::unordered_map<unsigned, partitionEntry> part_config;
  readPartitionConfig(part_config);
  //set scratchpad
  cerr << "Before Setting Scratchpad" << endl;
  for(auto it = part_config.begin(); it!= part_config.end(); ++it)
  {
    unsigned base_addr = it->first;
    unsigned size = it->second.array_size;
    unsigned p_factor = it->second.part_factor;
    cerr << base_addr << "," << size << "," << p_factor << endl;
    for ( unsigned i = 0; i < p_factor ; i++)
    {
      ostringstream oss;
      oss << base_addr << "-" << i;
      scratchpad->setScratchpad(oss.str(), size);
    }
  }
  cerr << "End Setting Scratchpad" << endl;
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
        oss << node_base << "-" << (int) (rel_addr / ceil (num_of_elements / p_factor)) ;
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
  write_gzip_string_file(bn + "_new_membase.gz", numTotalNodes, baseAddress);
}
//called in the end of the whole flow
void Datapath::dumpStats()
{
  writeFinalLevel();
  writeGlobalIsolated();
  writePerCycleActivity();
}

//localOptimizationFunctions
void Datapath::setGraphName(string graph_name)
{
  graphName = (char*)graph_name.c_str();
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
  igraph_t *tmp_g;
  tmp_g = new igraph_t;
  readGraph(tmp_g); 
  unsigned num_edges = (unsigned) igraph_ecount(tmp_g);
  unsigned num_nodes = (unsigned) igraph_vcount(tmp_g);
#ifdef DEBUG
  std::cerr << "num_edges," << num_edges << std::endl;
#endif
  std::vector<int> instid(num_nodes, 0);
  initInstID(instid);

  std::vector<int> methodid(num_nodes, 0);
  initMethodID(methodid);
  
  std::vector<unsigned> edge_latency(num_edges, 0);
  initEdgeLatency(edge_latency);
  
  std::vector<bool> tmp_isolated(num_nodes, 0);
  isolated_nodes(tmp_g, num_nodes, tmp_isolated);

  std::unordered_map<string, pair<int, int> > unrolling_config;
  readUnrollingConfig(unrolling_config);

  int add_unrolling_edges = 0;
  
  ofstream boundary;
  string file_name(graphName);
  file_name += "_loop_boundary";
  boundary.open(file_name.c_str());
  bool first = 0;
  for(unsigned node_id = 0; node_id < num_nodes; node_id++)
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
      
      igraph_vs_t vs_child;
      igraph_vit_t vit_child_it;
      igraph_vs_adj(&vs_child, node_id, IGRAPH_OUT);
      igraph_vit_create(tmp_g, vs_child, &vit_child_it);
      
      while(!IGRAPH_VIT_END(vit_child_it))
      {
        unsigned child_id = (unsigned)IGRAPH_VIT_GET(vit_child_it);
        int child_instid = instid.at(child_id);
        int child_methodid = methodid.at(child_id);
        IGRAPH_VIT_NEXT(vit_child_it);
        
        if ((child_instid == node_instid) 
           && (child_methodid == node_methodid))
        {
          int edge_id = check_edgeid(node_id, child_id, tmp_g);
          edge_latency[edge_id] = 1;
          microop.at(child_id) = IRINDEXADD;
          add_unrolling_edges++;
        }
      }
      igraph_vs_destroy(&vs_child);
      igraph_vit_destroy(&vit_child_it);
    }
    else if (microop.at(node_id) != IRINDEXADD)
      microop.at(node_id) = IRINDEXSTORE;
  }
  boundary << num_nodes << endl;
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
  igraph_t *tmp_g;
  tmp_g = new igraph_t;
  readGraph(tmp_g); 
  unsigned num_edges = (unsigned) igraph_ecount(tmp_g);
  unsigned num_nodes = (unsigned) igraph_vcount(tmp_g);
#ifdef DEBUG
  std::cerr << "num_edges," << num_edges << std::endl;
#endif
  std::vector<unsigned> address(num_nodes, 0);
  initAddress(address);
  
  std::vector<unsigned> edge_latency(num_edges, 0);
  std::vector<int> edge_parid(num_edges, 0);
  std::vector<int> edge_varid(num_edges, 0);

  initEdgeLatency(edge_latency);
  initEdgeParID(edge_parid);
  initEdgeVarID(edge_varid);
  
  std::vector<int> boundary;
  string file_name(graphName);
  file_name += "_loop_boundary";
  read_file(file_name, boundary);

  std::vector<bool> tmp_isolated(num_nodes, 0);
  isolated_nodes(tmp_g, num_nodes, tmp_isolated);
  
  std::vector<bool> to_remove_edges(num_edges, 0);
  std::vector<newEdge> to_add_edges;

  int shared_loads = 0;
  unsigned boundary_id = 0;
  int curr_boundary = boundary.at(boundary_id);
  
  int node_id = 0;
  while ( (unsigned)node_id < num_nodes)
  {
    std::unordered_map<unsigned, int> address_loaded;
    while (node_id < curr_boundary &&  (unsigned) node_id < num_nodes)
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
          
          igraph_vs_t vs_child;
          igraph_vit_t vit_child;
          igraph_vs_adj(&vs_child, node_id, IGRAPH_OUT);
          igraph_vit_create(tmp_g, vs_child, &vit_child);
          while (!IGRAPH_VIT_END(vit_child))
          {
            unsigned child_id = (int)IGRAPH_VIT_GET(vit_child);
            igraph_bool_t existed;
            igraph_are_connected(tmp_g, prev_load, child_id, &existed);
            int edge_id = check_edgeid(node_id, child_id, tmp_g);
            if (!existed)
            {
              newEdge *p = new newEdge[1];
              p->from = prev_load;
              p->to = child_id;
              p->varid = edge_varid.at(edge_id);
              p->parid = edge_parid.at(edge_id);
              p->latency = edge_latency.at(edge_id);
              to_add_edges.push_back(*p);
            }
            to_remove_edges[edge_id] = 1;
            IGRAPH_VIT_NEXT(vit_child);
          }
          igraph_vs_destroy(&vs_child);
          igraph_vit_destroy(&vit_child);

          igraph_vs_t vs_parent;
          igraph_vit_t vit_parent;
          igraph_vs_adj(&vs_parent, node_id, IGRAPH_IN);
          igraph_vit_create(tmp_g, vs_parent, &vit_parent);
          while(!IGRAPH_VIT_END(vit_parent))
          {
            int parent_id = (int)IGRAPH_VIT_GET(vit_parent);
            if (microop.at(parent_id) == IRADD ||
                microop.at(parent_id) == IRINDEXADD ||
                microop.at(parent_id) == IRINDEXSHIFT)
              microop.at(parent_id) = IRINDEXSTORE;
            IGRAPH_VIT_NEXT(vit_parent);
          }
          igraph_vs_destroy(&vs_parent);
          igraph_vit_destroy(&vit_parent);
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
  writeGraphWithIsolatedEdges(to_remove_edges);
  writeGraphWithNewEdges(to_add_edges);
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
  igraph_t *tmp_g;
  tmp_g = new igraph_t;
  readGraph(tmp_g); 
  unsigned num_edges = (unsigned) igraph_ecount(tmp_g);
  int num_nodes = (int) igraph_vcount(tmp_g);

  std::vector<int> edge_parid(num_edges, 0);
  std::vector<int> edge_varid(num_edges, 0);
  std::vector<unsigned> edge_latency(num_edges, 0);
  initEdgeLatency(edge_latency);
  initEdgeParID(edge_parid);
  initEdgeVarID(edge_varid);
  
  std::vector<int> boundary;
  string file_name(graphName);
  file_name += "_loop_boundary";
  read_file(file_name, boundary);
  
  std::vector<bool> tmp_isolated(num_nodes, 0);
  isolated_nodes(tmp_g, num_nodes, tmp_isolated);
  
  std::vector<bool> to_remove_edges(num_edges, 0);
  std::vector<newEdge> to_add_edges;

  int buffered_stores = 0;
  unsigned boundary_id = 0;
  int curr_boundary = boundary.at(boundary_id);
  
  int node_id = 0;
  while (node_id < num_nodes)
  {
    while (node_id < curr_boundary && node_id < num_nodes)
    {
      if(tmp_isolated.at(node_id))
      {
        node_id++;
        continue;
      }
      int node_microop = microop.at(node_id);
      if (is_store_op(node_microop))
      {
        igraph_vs_t vs_child;
        igraph_vit_t vit_child;
        igraph_vs_adj(&vs_child, node_id, IGRAPH_OUT);
        igraph_vit_create(tmp_g, vs_child, &vit_child);
        
        std::vector<unsigned> load_child;
        while (!IGRAPH_VIT_END(vit_child))
        {
          unsigned child_id = (int)IGRAPH_VIT_GET(vit_child);
          IGRAPH_VIT_NEXT(vit_child);
          int child_microop = microop.at(child_id);
          if (is_load_op(child_microop))
          {
            if (child_id >= (unsigned)curr_boundary )
              continue;
            else
              load_child.push_back(child_id);
          }
        }
        igraph_vs_destroy(&vs_child);
        igraph_vit_destroy(&vit_child);
        
        if (load_child.size() > 0)
        {
          buffered_stores++;
          igraph_vs_t vs_parents;
          igraph_vit_t vit_parents;
          igraph_vs_adj(&vs_parents, node_id, IGRAPH_IN);
          igraph_vit_create(tmp_g, vs_parents, &vit_parents);
          
          int store_parent = num_nodes;
          while (!IGRAPH_VIT_END(vit_parents))
          {
            unsigned parent_id = (int)IGRAPH_VIT_GET(vit_parents);
            int edge_id = check_edgeid(parent_id,  node_id, tmp_g);
            int parid = edge_parid.at(edge_id);
            //parent node that generates value
            if (parid == 3)
            {
              store_parent = parent_id;
              break;
            }
            IGRAPH_VIT_NEXT(vit_parents);
          }
          igraph_vs_destroy(&vs_parents);
          igraph_vit_destroy(&vit_parents);
          
          if (store_parent != num_nodes)
          {
            for (unsigned i = 0; i < load_child.size(); ++i)
            {
              unsigned load_id = load_child.at(i);
              igraph_vs_t vs_child;
              igraph_vit_t vit_child;
              igraph_vs_adj(&vs_child, load_id, IGRAPH_OUT);
              igraph_vit_create(tmp_g, vs_child, &vit_child);
              while(!IGRAPH_VIT_END(vit_child))
              {
                unsigned child_id = (unsigned)IGRAPH_VIT_GET(vit_child);
                int edge_id = check_edgeid(load_id, child_id, tmp_g);
                
                to_remove_edges.at(edge_id) = 1;
                //cerr << load_id << "," << child_id << endl;
                newEdge *p = new newEdge[1];
                p->from = store_parent;
                p->to = child_id;
                p->varid = edge_varid.at(edge_id);
                p->parid = edge_parid.at(edge_id);
                p->latency = edge_latency.at(edge_id);
                to_add_edges.push_back(*p);
                IGRAPH_VIT_NEXT(vit_child);
              }
              igraph_vs_destroy(&vs_child);
              igraph_vit_destroy(&vit_child);

              igraph_vs_t vs_ld_parents;
              igraph_vit_t vit_ld_parents;
              igraph_vs_adj(&vs_ld_parents, load_id, IGRAPH_IN);
              igraph_vit_create(tmp_g, vs_ld_parents, &vit_ld_parents);
              while(!IGRAPH_VIT_END(vit_ld_parents))
              {
                unsigned ld_parent_id = (unsigned)IGRAPH_VIT_GET(vit_ld_parents);
                int edge_id = check_edgeid(ld_parent_id, load_id, tmp_g);
                to_remove_edges.at(edge_id) = 1;
                IGRAPH_VIT_NEXT(vit_ld_parents);
              }
              igraph_vs_destroy(&vs_ld_parents);
              igraph_vit_destroy(&vit_ld_parents);
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
  writeGraphWithIsolatedEdges(to_remove_edges);
  writeGraphWithNewEdges(to_add_edges);
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
  igraph_t *tmp_g;
  tmp_g = new igraph_t;
  readGraph(tmp_g); 
  unsigned num_nodes = (unsigned) igraph_vcount(tmp_g);

  std::vector<unsigned> address(num_nodes, 0);
  initAddress(address);
  
  std::vector<bool> tmp_isolated(num_nodes, 0);
  isolated_nodes(tmp_g, num_nodes, tmp_isolated);
  
  std::vector<int> boundary;
  string file_name(graphName);
  file_name += "_loop_boundary";
  read_file(file_name, boundary);
  if (boundary.size() < 1)
    return;
  unordered_set<unsigned> to_remove_nodes;
  int shared_stores = 0;
  int boundary_id = boundary.size() - 1;
  int node_id = num_nodes -1;
  int next_boundary = boundary.at(boundary_id - 1);
  while (node_id >=0 )
  {
    unordered_map<unsigned, int> address_store_map;
    while (node_id >= next_boundary && node_id >= 0)
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
          igraph_vs_t vs_child;
          igraph_vit_t vit_child;
          igraph_vs_adj(&vs_child, node_id, IGRAPH_OUT);
          igraph_vit_create(tmp_g, vs_child, &vit_child);
          if (IGRAPH_VIT_SIZE(vit_child) == 0)
          {
            to_remove_nodes.insert(node_id);
            shared_stores++;
          }
          igraph_vs_destroy(&vs_child);
          igraph_vit_destroy(&vit_child);
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
}
/*
void Datapath::treeHeightReduction()
{
#ifdef DEBUG
  std::cerr << "=======Remove Shared Loads=====" << std::endl;
#endif
  //set graph
  igraph_t *tmp_g;
  tmp_g = new igraph_t;
  readGraph(tmp_g); 
  unsigned num_edges = (unsigned) igraph_ecount(tmp_g);
  unsigned num_nodes = (unsigned) igraph_vcount(tmp_g);
  
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
  
  std::vector<bool> tmp_isolated(num_nodes, 0);
  isolated_nodes(tmp_g, num_nodes, tmp_isolated);
  
  std::vector<bool> to_remove_edges(num_edges, 0);
  std::vector<newEdge> to_add_edges;
  
  unsigned boundary_id = 1;
  int used_vid = max_value(v_varid_4, 0, num_of_vertices);

  while (boundary_id <boundary.size() - 1)
  {
    unsigned end_inst = boundary.at(boundary_id);
    unsigned start_inst = boundary.at(boundary_id -1);
    if (start_inst >= num_nodes || end_inst>= num_nodes )
      break;
    vector<int> v_sort;
    partial_topological_sort(v_level, start_inst, end_inst, v_sort, tmp_isolated);
    vector<bool> updated(end_inst - start_inst, 0);
    for (unsigned i = 0; i < v_sort.size(); ++i)
    {
      int node_id = v_sort.at(i);
      if (v_isolated.at(node_id))
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
        unsigned node1 = num_nodes;
        unsigned node2 = num_nodes;
        int rank1 = num_nodes;
        int rank2 = num_nodes;
        unsigned min_rank = num_nodes;
        for (auto it = rank_map.begin(); it != rank_map.end(); ++it)
        {
          if (it->second < min_rank)
          {
            node1 = it->first;
            rank1 = it->second;
            min_rank = rank1;
          }
        }
        min_rank = num_nodes;
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
        assert((node1 != num_nodes) && (node2 != num_nodes));
        updated.at(*node_it - start_inst) = 1;
        
        new_par1id.at(*node_it) = par4vid.at(node1);
        new_par1value.at(*node_it) = par4value.at(node1);
        new_par1type.at(*node_it) = par1type.at(node1);
        
        newEdge *p = new newEdge[1];
        p->from = node1;
        p->to = *node_it;
        p->varid = new_par4vid.at(node1);
        p->parid = 1;
        p->latency = 1;
        to_add_edges.push_back(*p);

        new_par2id.at(*node_it) = par4vid.at(node2);
        new_par2value.at(*node_it) = par4value.at(node2);
        new_par2type.at(*node_it) = par1type.at(node2);
        
        newEdge *p = new newEdge[1];
        p->from = node2;
        p->to = *node_it;
        p->varid = new_par4vid.at(node2);
        p->parid = 2;
        p->latency = 1;
        to_add_edges.push_back(*p);
        
        new_par4id.at(*node_it) = ++used_vid;
        new_par4value.at(*node_it) = "";
        new_par4type.at(*node_it) = par1type.at(*node_it);
        
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
      p->varid = new_par4vid.at(node1);
      p->parid = 1;
      p->latency = 1;
      to_add_edges.push_back(*p);

      new_par2id.at(*node_it) = par4vid.at(node2);
      new_par2value.at(*node_it) = par4value.at(node2);
      new_par2type.at(*node_it) = par1type.at(node2);
      
      newEdge *p = new newEdge[1];
      p->from = node2;
      p->to = *node_it;
      p->varid = new_par4vid.at(node2);
      p->parid = 2;
      p->latency = 1;
      to_add_edges.push_back(*p);
      
      new_par4id.at(*node_it) = par4vid.at(node_id);
      new_par4value.at(*node_it) = par4value.at(node_id);
      new_par4type.at(*node_it) = par4type.at(node_id);
      
      rank_map.erase(node1);
      rank_map.erase(node2);
      
      int temp_rank = max(rank1, rank2) + 1;
      rank_map[*node_it] = temp_rank;
    }
    boundary_id++;
  }
  writeGraphWithIsolatedEdges(to_remove_edges);
  writeGraphWithNewEdges(to_add_edges);

  writeParType(new_par1type, 1);
  writeParType(new_par2type, 2);
  writeParType(new_par4type, 4);
  writeParVid(new_par1vid, 1);
  writeParVid(new_par2vid, 2);
  writeParVid(new_par4vid, 4);
  writeParValue(new_par1value, 1);
  writeParValue(new_par2value, 2);
  writeParValue(new_par4value, 4);
  
  cleanLeafNodes();
#ifdef DEBUG
  std::cerr << "=======End of Tree Height Reduction=====" << std::endl;
#endif

}
*/


//readWriteGraph
void Datapath::writeGraphWithNewEdges(std::vector<newEdge> &to_add_edges)
{
  string gn(graphName);
  string graph_file, edge_varid_file, edge_parid_file, edge_latency_file;
  graph_file = gn + "_graph";
  edge_varid_file = gn + "_edgevarid.gz";
  edge_parid_file = gn + "_edgeparid.gz";
  edge_latency_file = gn + "_edgelatency.gz";
  
  ofstream new_graph;
  ogzstream new_edgelatency, new_edgeparid, new_edgevarid;
  new_graph.open(graph_file.c_str(), std::ofstream::app);
  new_edgelatency.open(edge_latency_file.c_str(), std::ofstream::app);
  new_edgeparid.open(edge_parid_file.c_str(), std::ofstream::app);
  new_edgevarid.open(edge_varid_file.c_str(), std::ofstream::app);

  for(auto it = to_add_edges.begin(); it != to_add_edges.end(); ++it)
  {
    new_graph << it->from << " " << it->to << endl;
    new_edgelatency << it->latency << endl;
    new_edgeparid << it->parid << endl;
    new_edgevarid << it->varid << endl;
  }
  new_graph.close();
  new_edgelatency.close();
  new_edgeparid.close();
  new_edgevarid.close();
}
void Datapath::writeGraphWithIsolatedNodes(std::unordered_set<unsigned> &to_remove_nodes)
{
  igraph_t *tmp_g;
  tmp_g = new igraph_t;
  readGraph(tmp_g);
  
  unsigned num_edges = (unsigned) igraph_ecount(tmp_g);
  std::vector<unsigned> edge_latency(num_edges, 0);
  std::vector<int> edge_parid(num_edges, 0);
  std::vector<int> edge_varid(num_edges, 0);

  initEdgeLatency(edge_latency);
  initEdgeParID(edge_parid);
  initEdgeVarID(edge_varid);

  igraph_es_t es_edge;
  igraph_eit_t eit_edge_it;

  igraph_es_all(&es_edge, IGRAPH_EDGEORDER_ID);
  igraph_eit_create(tmp_g, es_edge, &eit_edge_it);
  IGRAPH_EIT_RESET(eit_edge_it);
  
  ofstream new_graph;
  ogzstream new_edgelatency, new_edgeparid, new_edgevarid;
  
  string gn(graphName);
  string graph_file, edge_varid_file, edge_parid_file, edge_latency_file;
  graph_file = gn + "_graph";
  edge_varid_file = gn + "_edgevarid.gz";
  edge_parid_file = gn + "_edgeparid.gz";
  edge_latency_file = gn + "_edgelatency.gz";
  
  new_graph.open(graph_file.c_str());
  new_edgelatency.open(edge_latency_file.c_str());
  new_edgeparid.open(edge_parid_file.c_str());
  new_edgevarid.open(edge_varid_file.c_str());

  while(!IGRAPH_EIT_END(eit_edge_it))
  {
    igraph_integer_t edge_id = IGRAPH_EIT_GET(eit_edge_it);
    IGRAPH_EIT_NEXT(eit_edge_it); 
    igraph_integer_t from, to;
    igraph_edge(tmp_g, edge_id, &from, &to);
    if (to_remove_nodes.find((int)from) != to_remove_nodes.end()
       || to_remove_nodes.find((int)to) != to_remove_nodes.end())
       continue;
    
    new_graph << (unsigned) from << " " << (unsigned) to << endl;
    new_edgelatency << edge_latency.at(edge_id) << endl;
    new_edgeparid << edge_parid.at(edge_id) << endl;
    new_edgevarid << edge_varid.at(edge_id) << endl;
  }
  igraph_es_destroy(&es_edge);
  igraph_eit_destroy(&eit_edge_it);

  new_graph.close();
  new_edgelatency.close();
  new_edgeparid.close();
  new_edgevarid.close();
}
void Datapath::writeGraphWithIsolatedEdges(std::vector<bool> &to_remove_edges)
{
//#ifdef DEBUG
  //std::cerr << "=======Write Graph With Isolated Edges=====" << std::endl;
//#endif
  igraph_t *tmp_g;
  tmp_g = new igraph_t;
  readGraph(tmp_g);
  
  unsigned num_edges = (unsigned) igraph_ecount(tmp_g);
  std::vector<unsigned> edge_latency(num_edges, 0);
  std::vector<int> edge_parid(num_edges, 0);
  std::vector<int> edge_varid(num_edges, 0);

  initEdgeLatency(edge_latency);
  initEdgeParID(edge_parid);
  initEdgeVarID(edge_varid);

  igraph_es_t es_edge;
  igraph_eit_t eit_edge_it;

  igraph_es_all(&es_edge, IGRAPH_EDGEORDER_ID);
  igraph_eit_create(tmp_g, es_edge, &eit_edge_it);
  IGRAPH_EIT_RESET(eit_edge_it);
  
  ofstream new_graph;
  ogzstream new_edgelatency, new_edgeparid, new_edgevarid;
  
  string gn(graphName);
  string graph_file, edge_varid_file, edge_parid_file, edge_latency_file;
  graph_file = gn + "_graph";
  edge_varid_file = gn + "_edgevarid.gz";
  edge_parid_file = gn + "_edgeparid.gz";
  edge_latency_file = gn + "_edgelatency.gz";
  
  new_graph.open(graph_file.c_str());
  new_edgelatency.open(edge_latency_file.c_str());
  new_edgeparid.open(edge_parid_file.c_str());
  new_edgevarid.open(edge_varid_file.c_str());

  while(!IGRAPH_EIT_END(eit_edge_it))
  {
    igraph_integer_t edge_id = IGRAPH_EIT_GET(eit_edge_it);
    IGRAPH_EIT_NEXT(eit_edge_it); 
    if (to_remove_edges.at(edge_id))
      continue;
    igraph_integer_t from, to;
    igraph_edge(tmp_g, edge_id, &from, &to);
    new_graph << (unsigned) from << " " << (unsigned) to << endl;
    new_edgelatency << edge_latency.at(edge_id) << endl;
    new_edgeparid << edge_parid.at(edge_id) << endl;
    new_edgevarid << edge_varid.at(edge_id) << endl;
  }
  igraph_es_destroy(&es_edge);
  igraph_eit_destroy(&eit_edge_it);

  new_graph.close();
  new_edgelatency.close();
  new_edgeparid.close();
  new_edgevarid.close();
//#ifdef DEBUG
  //std::cerr << "=======End Write Graph With Isolated Edges=====" << std::endl;
//#endif
}

void Datapath::readMethodGraph(igraph_t *tmp_g)
{
//#ifdef DEBUG
  //std::cerr << "=======Read Graph=====" << std::endl;
//#endif
  string gn(graphName);
  FILE *fp;
  string graph_file_name(gn + "_method_graph");
  fp = fopen(graph_file_name.c_str(), "r");
  if (!fp)
  {
    std::cerr << "no such file: " << graph_file_name << std::endl;
    exit(0);
  }
  igraph_read_graph_edgelist(tmp_g, fp, 0, 1);
  fclose(fp);
//#ifdef DEBUG
  //std::cerr << "=======End Read Graph=====" << std::endl;
//#endif
}


void Datapath::readGraph(igraph_t *tmp_g)
{
//#ifdef DEBUG
  //std::cerr << "=======Read Graph=====" << std::endl;
//#endif
  string gn(graphName);
  FILE *fp;
  string graph_file_name(gn + "_graph");
  fp = fopen(graph_file_name.c_str(), "r");
  if (!fp)
  {
    std::cerr << "no such file: " << graph_file_name << std::endl;
    exit(0);
  }
  igraph_read_graph_edgelist(tmp_g, fp, 0, 1);
  fclose(fp);
//#ifdef DEBUG
  //std::cerr << "=======End Read Graph=====" << std::endl;
//#endif
}

//initFunctions
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
  for (unsigned tmp_level = 0; tmp_level < cycle ; ++tmp_level)
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

  FILE *fp;
  string graph_file_name(gn + "_graph");
  fp = fopen(graph_file_name.c_str(), "r");
  if (!fp)
  {
    std::cerr << "no such file: " << graph_file_name << std::endl;
    exit(0);
  }
  std::cerr << "========Setting Graph======" << std::endl;
  igraph_read_graph_edgelist(g, fp, 0, 1);
  fclose(fp);
  
  numGraphNodes = (unsigned) igraph_vcount(g);
  numGraphEdges = (unsigned) igraph_ecount(g);
  
  edgeLatency.assign(numGraphEdges, 0);
  read_gzip_file(gn + "_edgelatency.gz", numGraphEdges, edgeLatency);
  
  isolated.assign(numGraphNodes, 0);
  isolated_nodes(g, numGraphNodes, isolated);
  
  updateGlobalIsolated();

  update_method_latency(benchName, callLatency);
  numParents.assign(numGraphNodes, 0);
  totalConnectedNodes = initialized_num_of_parents(g, isolated, numParents);
  cerr << "totalConnectedNodes," << totalConnectedNodes << endl;

  executedNodes = 0;

  prevCycle = cycle;

  memReadyQueue.clear();
  nonMemReadyQueue.clear();
  executedQueue.clear();

  initReadyQueue();
  cerr << "End of Setting Graph: " << graph_name << endl;
}

int Datapath::clearGraph()
{
  string gn(graphName);

  cerr << gn << "," << cycle-prevCycle << endl;

  edgeLatency.clear();
  callLatency.clear();
  numParents.clear();
  isolated.clear();
  std::vector<int>().swap(edgeLatency);
  std::vector<bool>().swap(isolated);
  std::vector<int>().swap(numParents);

  igraph_destroy(g);
  return cycle-prevCycle;
}

bool Datapath::step()
{
  cerr << "===========Stepping============" << endl;
  int firedNodes = fireNonMemNodes();
  firedNodes += fireMemNodes();

  stepExecutedQueue();

  cerr << "Cycle:" << cycle << ",executedNodes," << executedNodes << ",totalConnectedNodes," << totalConnectedNodes << endl;
  cycle++;
  if (executedNodes == totalConnectedNodes)
    return 1;
  return 0;
}

void Datapath::stepExecutedQueue()
{
  auto it = executedQueue.begin();
  while (it != executedQueue.end())
  {
    //it->second is the number of cycles to execute current nodes
    //cerr << "executing," << it->first << "," << microop.at(it->first) << "," << it->second << endl;
    if (it->second <= cycleTime)
    {
      unsigned node_id = it->first;
      executedNodes++;
      newLevel.at(node_id) = cycle;
      updateChildren(node_id, it->second);
      it = executedQueue.erase(it);
    }
    else
    {
      it->second -= cycleTime;
      it++;
    }
  }
}

void Datapath::updateChildrenForNextStep(unsigned node_id)
{
  igraph_vs_t vs_children;
  igraph_vit_t vit_children;
  igraph_vs_adj(&vs_children, node_id, IGRAPH_OUT);
  igraph_vit_create(g, vs_children, &vit_children);
  while (!IGRAPH_VIT_END(vit_children))
  {
    unsigned child_id = (unsigned) IGRAPH_VIT_GET(vit_children);
    if (numParents[child_id] > 0)
    {
      numParents[child_id]--;
      if (numParents[child_id] == 0)
      {
        if (is_memory_op(microop.at(child_id)))
          addMemReadyNode(child_id);
        else
          addNonMemReadyNode(child_id);
      }
    }
    IGRAPH_VIT_NEXT(vit_children);
  }
  igraph_vs_destroy(&vs_children);
  igraph_vit_destroy(&vit_children);
}


void Datapath::updateChildrenForCurrentStep(unsigned node_id)
{
  igraph_vs_t vs_children;
  igraph_vit_t vit_children;
  igraph_vs_adj(&vs_children, node_id, IGRAPH_OUT);
  igraph_vit_create(g, vs_children, &vit_children);
  while (!IGRAPH_VIT_END(vit_children))
  {
    unsigned child_id = (unsigned) IGRAPH_VIT_GET(vit_children);
    if (numParents[child_id] == 1)
    {
      int edge_id = check_edgeid(node_id, child_id, g);
      if (edgeLatency.at(edge_id) == 0)
      {
        //If doable in current cycle, do it; otherwise, don't touch
        //anything
        numParents[child_id] = 0;
        //child node latency
        //currently assuming everyone is one cycle
        if (is_memory_op(microop.at(child_id)))
          addMemReadyNode(child_id);
        else
          addNonMemReadyNode(child_id);
      }
    }
    IGRAPH_VIT_NEXT(vit_children);
  }
  igraph_vs_destroy(&vs_children);
  igraph_vit_destroy(&vit_children);

}

void Datapath::updateChildren(unsigned node_id, float latencySoFar)
{
  igraph_vs_t vs_children;
  igraph_vit_t vit_children;
  igraph_vs_adj(&vs_children, node_id, IGRAPH_OUT);
  igraph_vit_create(g, vs_children, &vit_children);
  while (!IGRAPH_VIT_END(vit_children))
  {
    unsigned child_id = (unsigned) IGRAPH_VIT_GET(vit_children);
    if (numParents[child_id] > 0)
    {
      numParents[child_id]--;
      if (numParents[child_id] == 0)
      {
        if (is_memory_op(microop.at(child_id)))
          addMemReadyNode(child_id);
        else
          executedQueue.push_back(make_pair(child_id, latencySoFar + node_latency(microop.at(child_id))));
      }
    }
    IGRAPH_VIT_NEXT(vit_children);
  }
  igraph_vs_destroy(&vs_children);
  igraph_vit_destroy(&vit_children);

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
  cerr << "fired Memory Nodes," << firedNodes << endl;
  return firedNodes;
}

int Datapath::fireNonMemNodes()
{
  cerr << "=========Firing NonMemory Nodes========" << endl;
  int firedNodes = 0;
  //assume the Queue is sorted by somehow
  //non considering user's constraints on num of functional units
  auto it = nonMemReadyQueue.begin();
  while (it != nonMemReadyQueue.end())
  {
    unsigned node_id = *it;
    firedNodes++;
    //if call instruction, put in executedQueue and start countdown
    //else update its children
    if (callLatency.find(node_id) != callLatency.end())
    {
      int method_cycle = callLatency[node_id];
      executedQueue.push_back(make_pair(node_id, method_cycle-1));
    }
    else
    {
      //find its children that can execute in the same cycle
      //FIXME floating point node latency, check cycle boundary
      executedQueue.push_back(make_pair(node_id, node_latency(microop.at(node_id))));
      updateChildren(node_id, node_latency(microop.at(node_id)));
    }
    it = nonMemReadyQueue.erase(it);
  }
  cerr << "Fired Non-Memory Nodes: " << firedNodes << endl;
  return firedNodes;
}

void Datapath::initReadyQueue()
{
  cerr << "======Initializing Ready Queue=========" << endl;
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
  cerr << "InitialReadyQueueSize: Memory," << memReadyQueue.size() << ", Non-Mem," << nonMemReadyQueue.size() << endl;
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
void Datapath::readUnrollingConfig(std::unordered_map<string, pair<int, int> > &unrolling_config)
{
  ifstream config_file;
  string file_name(benchName);
  file_name += "_unrolling_config";
  config_file.open(file_name.c_str());
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
    cerr << oss.str() << "," << factor << endl;
  }
  config_file.close();
}
void Datapath::readInductionConfig(std::unordered_set<string> &ind_config)
{
  ifstream config_file;
  string file_name(benchName);
  file_name += "_induction_variables";
  config_file.open(file_name.c_str());
  while(!config_file.eof())
  {
    string wholeline;
    getline(config_file, wholeline);
    if (wholeline.size() == 0)
      break;
    char fun[256];
    int methodid, varid, baseid, instid, loopid;
    sscanf(wholeline.c_str(), "%[^,],%d,%d,%d,%d,%d\n", fun,&methodid, &varid, &baseid, &instid, &loopid);
    
    ostringstream oss;
    oss << methodid << "-" << varid ;
    cerr << "ind_config inserting: " << oss.str() << endl;
    ind_config.insert(oss.str());
  }
  config_file.close();

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
      
      partitionEntry *p = new partitionEntry[1];
      p->type = p_type;
      p->array_size = size ;
      p->part_factor = p_factor;
      partition_config[base_addr] = *p;
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
/*
void Datapath::findAllAssociativeChildren(igraph_t *g, int node_id, 
  unsigned lower_bound, list<int> &nodes, 
  vector<int> &leaves, vector<int> &leaves_rank, vector<pair<int, int>> &remove_edges)
{
  if (is_associative(v_microop.at(node_id)))
  {
    nodes.push_front(node_id);
    igraph_vs_t vs_parents;
    igraph_vit_t vit_parents;
    igraph_vs_adj(&vs_parents, node_id, IGRAPH_IN);
    igraph_vit_create(g, vs_parents, &vit_parents);
    while(!IGRAPH_VIT_END(vit_parents))
    {
      unsigned parent_id = (int)IGRAPH_VIT_GET(vit_parents);
      if (parent_id >= lower_bound)
      {
        int parent_microop = v_microop.at(parent_id);
        if (is_associative(parent_microop) && has_one_child(g, parent_id))
        {
          remove_edges.push_back(make_pair(parent_id, node_id));
          findAllAssociativeChildren(g, parent_id, lower_bound,
            nodes, leaves, leaves_rank, remove_edges);
        }
        else
        {
          remove_edges.push_back(make_pair(parent_id, node_id));
          leaves.push_back(parent_id);
          leaves_rank.push_back(0);
        }
      }
      else
      {
        remove_edges.push_back(make_pair(parent_id, node_id));
        leaves.push_back(parent_id);
        leaves_rank.push_back(1);
      }
      IGRAPH_VIT_NEXT(vit_parents);
    }
  }
  else
  {
    leaves.push_back(node_id);
    leaves_rank.push_back(0);
  }
}
*/
