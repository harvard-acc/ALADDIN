#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

int getEdgeWeight(ScratchpadDatapath* acc,
                  Graph& graph,
                  unsigned node_id_0,
                  unsigned node_id_1) {
  EdgeNameMap edge_to_parid = get(boost::edge_name, graph);
  ExecNode* node_0 = acc->getNodeFromNodeId(node_id_0);
  ExecNode* node_1 = acc->getNodeFromNodeId(node_id_1);
  auto edge_pair = edge(node_0->get_vertex(), node_1->get_vertex(), graph);
  if (!edge_pair.second)
    return -1;
  return edge_to_parid[edge_pair.first];
}

/* Code:
 *
 * int result[10];
 * int input0[8];
 * input input1[8];
 *
 * outer:
 * for (int i = 0; i < 10; i++) {
 *   inner:
 *   for (int j = 0; j < 8; j++) {
 *     result[i] += input0[j] * input1[j];
 *   }
 * }
 *
 */
SCENARIO("Test register load/store fusion", "[reg_ls_fusion]") {
  GIVEN("Doubly-nested loop with a multiply-accumulate operation in "
        "the innermost loop") {
    std::string bench("outputs/reg_ls_fusion");
    std::string trace_file("inputs/reg-ls-fusion-trace.gz");
    std::string config_file("inputs/config-reg-ls-fusion");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    acc->removePhiNodes();
    acc->removeInductionDependence();
    acc->initBaseAddress();
    acc->completePartition();
    acc->scratchpadPartition();
    acc->loopFlatten();
    acc->loopUnrolling();
    acc->removeSharedLoads();
    acc->storeBuffer();
    acc->removeRepeatedStores();
    acc->memoryAmbiguation();
    acc->treeHeightReduction();

    WHEN("Before applying register load/store fusion") {
      THEN("Inner loop should contain chains of Load, Add, Store, connected by "
           "memory references.") {
        REQUIRE(acc->getMicroop(10) == LLVM_IR_Load);
        REQUIRE(acc->getMicroop(11) == LLVM_IR_Add);
        REQUIRE(acc->getMicroop(12) == LLVM_IR_Store);
        REQUIRE(acc->getMicroop(23) == LLVM_IR_Load);
        REQUIRE(acc->getMicroop(24) == LLVM_IR_Add);
        REQUIRE(acc->getMicroop(25) == LLVM_IR_Store);
        REQUIRE(acc->getMicroop(49) == LLVM_IR_Load);
        REQUIRE(acc->getMicroop(50) == LLVM_IR_Add);
        REQUIRE(acc->getMicroop(51) == LLVM_IR_Store);

        Graph graph = acc->getGraph();
        REQUIRE(getEdgeWeight(acc, graph, 10, 11) == 1);
        REQUIRE(getEdgeWeight(acc, graph, 11, 12) == 1);
        REQUIRE(getEdgeWeight(acc, graph, 23, 24) == 1);
        REQUIRE(getEdgeWeight(acc, graph, 24, 25) == 1);
        REQUIRE(getEdgeWeight(acc, graph, 49, 50) == 1);
        REQUIRE(getEdgeWeight(acc, graph, 50, 51) == 1);
      }
    }

    acc->fuseRegLoadStores();

    WHEN("After applying register load/store fusion") {
      THEN("Edge weights should be set to REGISTER_EDGE") {
        Graph graph = acc->getGraph();
        REQUIRE(getEdgeWeight(acc, graph, 10, 11) == REGISTER_EDGE);
        REQUIRE(getEdgeWeight(acc, graph, 11, 12) == REGISTER_EDGE);
        REQUIRE(getEdgeWeight(acc, graph, 23, 24) == REGISTER_EDGE);
        REQUIRE(getEdgeWeight(acc, graph, 24, 25) == REGISTER_EDGE);
        REQUIRE(getEdgeWeight(acc, graph, 49, 50) == REGISTER_EDGE);
        REQUIRE(getEdgeWeight(acc, graph, 50, 51) == REGISTER_EDGE);
      }
    }

    WHEN("After ASAP scheduling") {
      acc->prepareForScheduling();
      while (!acc->step()) {}

      THEN("The load should execute one cycle before the add and the store, "
           "and the add and store should execute simultaneously") {
        unsigned node_10_time =
            acc->getNodeFromNodeId(10)->get_complete_execution_cycle();
        REQUIRE(acc->getNodeFromNodeId(11)->get_complete_execution_cycle() ==
                node_10_time + 1) ;
        REQUIRE(acc->getNodeFromNodeId(12)->get_complete_execution_cycle() ==
                node_10_time + 1);
        unsigned node_23_time =
            acc->getNodeFromNodeId(23)->get_complete_execution_cycle();
        REQUIRE(acc->getNodeFromNodeId(24)->get_complete_execution_cycle() ==
                node_23_time + 1);
        REQUIRE(acc->getNodeFromNodeId(25)->get_complete_execution_cycle() ==
                node_23_time + 1);
        unsigned node_49_time =
            acc->getNodeFromNodeId(49)->get_complete_execution_cycle();
        REQUIRE(acc->getNodeFromNodeId(50)->get_complete_execution_cycle() ==
                node_49_time + 1);
        REQUIRE(acc->getNodeFromNodeId(51)->get_complete_execution_cycle() ==
                node_49_time + 1);
      }
    }
  }
}
