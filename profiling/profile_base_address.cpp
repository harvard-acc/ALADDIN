#include "profile_base_address.h"
/*
Input File:
  graph_file_name: original graph
  microop_file_name: stores the microop per dynamic node
  par1valuee_file_name: stores the value of par 1 per dynamic node
Output File:
  mem_base_file_name, store base address of memory operation
*/

int profile_base_address(string bench, string base_addr_name)
{
	
  igraph_t g;
	FILE *fp;
  string graph_file_name(bench);

  graph_file_name += "_graph";
	fp = fopen(graph_file_name.c_str(), "r");
	
  if(!fp)
  {
		std::cerr << "file not opened" << std::endl;
		return 1;
	}
	else
  {
    std::cerr << "=================Base Addr Profile===================" 
              << std::endl; 
  }
	
  igraph_read_graph_edgelist(&g, fp, 0, 1);
	fclose(fp);	
	

  /*Get basic graph stats*/
  unsigned num_of_vertices = (unsigned) igraph_vcount(&g);
  //unsigned num_of_edges  = (unsigned ) igraph_ecount(&g);
	
  ifstream base_addr_file;
  base_addr_file.open(base_addr_name);
  vector<unsigned> base_addr;
  while (!base_addr_file.eof())
  {
    string wholeline;
    getline(base_addr_file, wholeline);
    if (wholeline.size() == 0)
      break;
    istringstream parser(wholeline, istringstream::in);
    unsigned var, varid, int_type, size, addr;
    char comma;
    parser >> var >> comma >> varid >> comma >> int_type >> comma >> size >> comma >> addr;
    base_addr.push_back(addr);
  }
  base_addr_file.close();
  sort(base_addr.begin(), base_addr.end());
    
  /*Read par1 value*/
  vector<unsigned> v_par1value(num_of_vertices, -1);
  string par1value_file_name(bench);
  par1value_file_name += "_par1value.gz";
  read_gzip_unsigned_file(par1value_file_name, num_of_vertices, v_par1value);
  
  /*Read microop id */
  vector<int> v_microop(num_of_vertices, 0);
  string microop_file_name(bench);
  microop_file_name += "_microop.gz";
  read_gzip_file(microop_file_name, num_of_vertices, v_microop);
	
  vector<unsigned> v_membase(num_of_vertices, 0);
  /*
  IGRAPH_OUT: nodes with no incoming edges go first
  IGRAPH_IN: nodes with no outgoing edges go first
  */
  /*
  using IGRAPH_IN we find the children with derived induction variable first
  then try to find their parents which define the basic induction variable
  */
  igraph_vector_t v_topological;
	igraph_vector_init(&v_topological, num_of_vertices);
	igraph_topological_sorting(&g, &v_topological, IGRAPH_OUT);

	for (unsigned i = 0; i < num_of_vertices ; i++)
  {
    int updating_node				= (int) VECTOR(v_topological)[i];
    int updating_node_microop 		= v_microop.at(updating_node);
    if (updating_node_microop == IRLOADREL || updating_node_microop == IRSTOREREL )
    {
      bool updated = 0;
      igraph_vs_t vs_parent;
      igraph_vit_t vit_parent_it;
      igraph_vs_adj(&vs_parent, updating_node, IGRAPH_IN);	
      igraph_vit_create(&g, vs_parent, &vit_parent_it);
      while(!IGRAPH_VIT_END(vit_parent_it))
      {
        unsigned parent_id	= (unsigned) IGRAPH_VIT_GET(vit_parent_it);
        unsigned parent_microop	= v_microop.at(parent_id);
        if (parent_microop == IRGETADDRESS)
        {
          //not real memory operation
          updated = 1;
          v_microop.at(updating_node) = IRSTORE;
          break;
        }
        else 
        {
          unsigned abs_addr = v_par1value.at(updating_node);
          for (unsigned i = 0; i < base_addr.size(); i++)
          {
            if ( i == 0)
              assert(abs_addr >= base_addr.at(i));
            else
            {
              if (abs_addr < base_addr.at(i) && abs_addr >= base_addr.at(i-1))
              {
                v_membase.at(updating_node) = base_addr.at(i-1);
                break;
              }
              else if (i == base_addr.size() -1)
                v_membase.at(updating_node) = base_addr.at(i);
            }
          }
          updated = 1;
        }
        IGRAPH_VIT_NEXT(vit_parent_it);
      }
      igraph_vs_destroy(&vs_parent);
      igraph_vit_destroy(&vit_parent_it);
      if (!updated)
        v_membase.at(updating_node) = v_par1value.at(updating_node);
    }
	}
  igraph_vector_destroy(&v_topological);
  igraph_destroy(&g);
  string mem_base_file_name(bench);
  mem_base_file_name += "_membase.gz";
  write_gzip_unsigned_file(mem_base_file_name, num_of_vertices, v_membase);
  write_gzip_file(microop_file_name, num_of_vertices, v_microop);
  
  std::cerr << "=================END Base Address Profile===============" << std::endl;
  std::cerr << std::endl;
	return 0;
}



