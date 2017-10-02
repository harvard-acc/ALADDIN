#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

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
    auto& prog = acc->getProgram();
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
        REQUIRE(prog.nodes.at(10)->get_microop() == LLVM_IR_Load);
        REQUIRE(prog.nodes.at(11)->get_microop() == LLVM_IR_Add);
        REQUIRE(prog.nodes.at(12)->get_microop() == LLVM_IR_Store);
        REQUIRE(prog.nodes.at(23)->get_microop() == LLVM_IR_Load);
        REQUIRE(prog.nodes.at(24)->get_microop() == LLVM_IR_Add);
        REQUIRE(prog.nodes.at(25)->get_microop() == LLVM_IR_Store);
        REQUIRE(prog.nodes.at(49)->get_microop() == LLVM_IR_Load);
        REQUIRE(prog.nodes.at(50)->get_microop() == LLVM_IR_Add);
        REQUIRE(prog.nodes.at(51)->get_microop() == LLVM_IR_Store);

        REQUIRE(prog.getEdgeWeight(10, 11) == 1);
        REQUIRE(prog.getEdgeWeight(11, 12) == 1);
        REQUIRE(prog.getEdgeWeight(23, 24) == 1);
        REQUIRE(prog.getEdgeWeight(24, 25) == 1);
        REQUIRE(prog.getEdgeWeight(49, 50) == 1);
        REQUIRE(prog.getEdgeWeight(50, 51) == 1);
      }
    }

    acc->fuseRegLoadStores();

    WHEN("After applying register load/store fusion") {
      THEN("Edge weights should be set to REGISTER_EDGE") {
        REQUIRE(prog.getEdgeWeight(10, 11) == REGISTER_EDGE);
        REQUIRE(prog.getEdgeWeight(11, 12) == REGISTER_EDGE);
        REQUIRE(prog.getEdgeWeight(23, 24) == REGISTER_EDGE);
        REQUIRE(prog.getEdgeWeight(24, 25) == REGISTER_EDGE);
        REQUIRE(prog.getEdgeWeight(49, 50) == REGISTER_EDGE);
        REQUIRE(prog.getEdgeWeight(50, 51) == REGISTER_EDGE);
      }
    }

    WHEN("After ASAP scheduling") {
      acc->prepareForScheduling();
      while (!acc->step()) {}

      THEN("The load should execute one cycle before the add and the store, "
           "and the add and store should execute simultaneously") {
        unsigned node_10_time =
            prog.nodes.at(10)->get_complete_execution_cycle();
        REQUIRE(prog.nodes.at(11)->get_complete_execution_cycle() ==
                node_10_time + 1);
        REQUIRE(prog.nodes.at(12)->get_complete_execution_cycle() ==
                node_10_time + 1);
        unsigned node_23_time =
            prog.nodes.at(23)->get_complete_execution_cycle();
        REQUIRE(prog.nodes.at(24)->get_complete_execution_cycle() ==
                node_23_time + 1);
        REQUIRE(prog.nodes.at(25)->get_complete_execution_cycle() ==
                node_23_time + 1);
        unsigned node_49_time =
            prog.nodes.at(49)->get_complete_execution_cycle();
        REQUIRE(prog.nodes.at(50)->get_complete_execution_cycle() ==
                node_49_time + 1);
        REQUIRE(prog.nodes.at(51)->get_complete_execution_cycle() ==
                node_49_time + 1);
      }
    }
  }
}
