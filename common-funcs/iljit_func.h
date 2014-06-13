#ifndef ILJIT_FUNC_H
#define ILJIT_FUNC_H

#include <iostream> 
#include <sstream> // std::istringstream
#include <fstream>
#include <stdlib.h>
#include <gzstream.h>
#include <string> 
#include <vector>
#include <assert.h>
#include <unordered_map>
#include "./node_latency.h"
#include "/group/brooks/ildjit/src/libiljitu/src/ir_language.h"

#define IRINDEXADD 100
#define IRINDEXSHIFT 101
#define IRINDEXSTORE 102
using namespace std;

bool is_index_inst(unsigned microop);
bool is_associative (unsigned microop);
bool is_branch_inst(unsigned microop);
bool is_compare_inst(unsigned microop);
bool is_integer(unsigned partype);
bool is_unsigned_integer(unsigned partype);
float node_latency (unsigned  microop);
float new_node_latency(int child_id, int microop, unordered_map<int, float> &call_latency);
bool is_memory_op ( unsigned microop);
bool is_load_op ( unsigned microop);
bool is_store_op ( unsigned microop);
bool is_index_adder(unsigned microop);

#endif
