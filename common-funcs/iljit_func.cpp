#include "iljit_func.h"

bool is_associative (unsigned microop)
{
  if ( (microop == IRADD) || (microop == IRADDOVF) )
    return true;
  return false;
}

/*
Input: microop id
Output: return true if microop == IRBRANCH/IRBRANCHIF/IRBRANCHIFNOT/IRLABEL
*/
bool is_branch_inst(unsigned microop)
{ 
  switch(microop)
  {
    case IRBRANCH: case IRBRANCHIF: case IRBRANCHIFNOT: case IRLABEL:
      return 1;
    default:
      return 0;
  }
}
bool is_index_inst(unsigned microop)
{ 
  switch(microop)
  {
    case IRINDEXADD: case IRINDEXSHIFT: case IRINDEXSTORE: case IRSTORE:
      return 1;
    default:
      return 0;
  }
}

bool is_compare_inst(unsigned microop)
{
  switch(microop)
  {
    case IRGT: case IREQ: case IRLT:
      return 1;
    default:
      return 0;
  }
}
/*return true is partype is IRINT*/
bool is_integer(unsigned partype)
{
  switch(partype)
  {
    case IRINT8: case IRINT16: case IRINT32: case IRINT64: case IRNINT:
      return 1;
    default:
      return 0;
  }
}
/*return true is partype is IRUINT*/
bool is_unsigned_integer(unsigned partype)
{
  switch(partype)
  {
    case IRUINT8: case IRUINT16: case IRUINT32: case IRUINT64: case IRNUINT:
      return 1;
    default:
      return 0;
  }
}

float node_latency (unsigned  microop)
{
  switch(microop)
  {
    case IRADD: case IRADDOVF: case IRSUB: case IRSUBOVF: case IRINDEXADD:
      return ADD_LATENCY;
    case IRMUL: case IRMULOVF: case IRDIV:  
      return MUL_LATENCY;
    case IRLOADREL: case IRSTOREREL: case IRLOADELEM: case IRSTOREELEM: case
    IRRET:
      return MEMOP_LATENCY;
    default:
      return 0;
  }

}

float new_node_latency(int child_id, int microop, unordered_map<int, float> &call_latency)
{
  auto it = call_latency.find(child_id);
  if (it == call_latency.end())
    return node_latency(microop);
  return call_latency[child_id];
}

//return true if microop is a memory operation, including both load and store
bool is_memory_op ( unsigned microop)
{
  switch (microop)
  {
    case IRLOADREL: case IRSTOREREL : 
    case IRLOADELEM : case IRSTOREELEM :
      return true;
    default:
      return false;
  }
}

bool is_store_op ( unsigned microop)
{
  switch (microop)
  {
    case IRSTOREREL : 
    case IRSTOREELEM :
      return true;
    default:
      return false;
  }
}
bool is_load_op ( unsigned microop)
{
  switch (microop)
  {
    case IRLOADREL: case IRLOADELEM : 
      return true;
    default:
      return false;
  }
}

bool is_index_adder(unsigned microop)
{
  if (microop == IRINDEXADD)
    return true;
  return false;
}
