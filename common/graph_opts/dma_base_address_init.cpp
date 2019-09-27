#include "../DDDG.h"
#include "dma_base_address_init.h"

// Determine the base addresses and array names of the DMA access operands.
//
// DMA version v3 does not require the user to specify the source and
// destination addresses separately as base and offset. The base address is
// required to properly translate the trace address to the simulation address
// by mapping it to an array name, and if the DMA access contains an address
// that is not the base address (aka array[0], index 0), translation would not
// be possible.
//
// This optimization pass traces the address operands back to the original node
// that contains the base address and name of the array being accessed and
// assigns the source and destination variable pointers of the DMA node
// accordingly.
//
// Example:  Suppose the user writes the following C code, wanting to load data
// from a specific place on the host into an accelerator-local scratchpad:
//
//   void my_kernel(int* host_array, int* accel_array, int size) {
//     dmaLoad(&accel_array[0], &host_array[4], size);
//     /* does some work, using ONLY accel_array */
//     dmaStore(&host_array[4], &accel_array[0], size);
//   }
//
//   int main() {
//     mapArrayToAccelerator(ACCEL_ID, "host_array", host_array, size);
//     my_kernel(host_array, accel_array, size);
//     return 0;
//   }
//
// And (part of) the Aladdin configuration file like this (note that host_array
// does not to be declared here):
//
//   partition,cyclic,accel_array,16,4,1
//
// LLVM-Tracer will generate an entry block and dmaLoad node for this function:
//
//   entry,my_kernel,3,
//   1,64,0xffff0000,1,host_array   <- This is a pointer type (0x prefix).
//   2,64,0xeeee0000,1,accel_array  <- Also a pointer type.
//   3,32,16,1,size                 <- Plain old integer.
//
//   0,43,my_kernel,0:0,7,29,0
//   2,64,4,1,4,
//   1,64,0xffff0000,1,host_array,
//   r,64,0xffff0010,1,7,      <- &host_addr[4] stored to reg 7.
//
//   0,42,my_kernel,0:0,6,99,1
//   4,64,0x40db80,1,dmaLoad,
//   1,64,0xeeee0000,1,accel_array,
//   2,64,0xffff0010,1,7,      <- label is "7", not "host_array".
//   3,64,16,1,size,
//   f,64,0xeeee0000,1,dst_addr,
//   f,64,0xffff0010,1,src_host_addr,
//   f,64,16,1,size,
//   r,32,3,1,6,
//
// The entry block tells us the trace base address of host_array, and the
// mapArrayToAccelerator() call associates the "host_array" label with the
// simulation virtual address of the host_array variable. This DMA base address
// initialization pass will attach the correct Variable objects to the DMA
// node by tracing the source and destination operands back to the GEP nodes
// that generated the final addresses (in this example, node 0). Now, during
// address translation, we can correctly determine that address 0xffff0010,
// stored in reg 7, refers to the array "host_array", based at 0xffff0000,
// instead of the array "7".

using namespace SrcTypes;

std::string DmaBaseAddressInit::getCenteredName(size_t size) {
  return "     Init DMA Base Address     ";
}

// Set the memory type of this host memory access node.
void DmaBaseAddressInit::setHostMemoryType(ExecNode* node) {
  HostMemAccess* mem_access = node->get_host_mem_access();
  Variable* src_var = mem_access->src_var;
  Variable* dst_var = mem_access->dst_var;
  assert(src_var->get_name() != dst_var->get_name() &&
         "The local and host arrays must not have the same name!");
  const std::string& array_label =
      node->is_host_load() ? src_var->get_name() : dst_var->get_name();
  auto it = user_params.partition.find(array_label);
  if (it != user_params.partition.end())
    mem_access->memory_type = it->second.memory_type;
}

void DmaBaseAddressInit::optimize() {
  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);

  for (auto& node_it : exec_nodes) {
    ExecNode* node = node_it.second;
    if (!node->is_host_mem_op())
      continue;

    Vertex dma_vertex = node->get_vertex();

    // Find the node that generated this address, if it exists.
    bool found_src = false, found_dst = false;
    in_edge_iter in_edge_it, in_edge_end;
    for (boost::tie(in_edge_it, in_edge_end) = in_edges(dma_vertex, graph);
         in_edge_it != in_edge_end;
         ++in_edge_it) {
      int edge_parid = edge_to_parid[*in_edge_it];
      unsigned parent_id = vertex_to_name[source(*in_edge_it, graph)];
      ExecNode* parent_node = exec_nodes.at(parent_id);
      // The edge weight between a DMA node and a GEP node is either 1 (for the
      // destination address) or 2 (for the source address). The values refer
      // to which function call parameter each argument was.
      if (!(parent_node->is_gep_op() && (edge_parid == 1 || edge_parid == 2 ||
                                         edge_parid == MEMORY_EDGE))) {
        if (node->is_host_load() || node->is_host_store())
          setHostMemoryType(node);
        continue;
      }

      DynamicVariable dynvar = parent_node->get_dynamic_variable();
      DynamicVariable orig_var = call_argument_map.lookup(dynvar);
      if (node->is_host_load() || node->is_host_store()) {
        HostMemAccess* mem_access = node->get_host_mem_access();
        if (edge_parid == 1) {
          mem_access->dst_var = orig_var.get_variable();
          found_dst = true;
        } else if (edge_parid == 2) {
          mem_access->src_var = orig_var.get_variable();
          found_src = true;
        }
        setHostMemoryType(node);
        if (found_src && found_dst) {
          break;
        }
      } else if (node->is_set_ready_bits()) {
        ReadyBitAccess* mem_access = node->get_ready_bit_access();
        mem_access->array = orig_var.get_variable();
        found_src = found_dst = true;
      }
    }
  }
}
