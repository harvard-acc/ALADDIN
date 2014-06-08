#ifndef PROFILE_BASE_ADDRESS_H
#define PROFILE_BASE_ADDRESS_H

#include "file_func.h"
#include "iljit_func.h"
/*#include "graph_func.h"*/
#include <algorithm>
#include <boost/graph/graphviz.hpp>
#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/topological_sort.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
using namespace std;

int profile_base_address(string bench, string base_addr_name);
#endif
