#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

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
        REQUIRE(prog.nodes.at(22)->get_microop() == LLVM_IR_Load);
        REQUIRE(prog.nodes.at(23)->get_microop() == LLVM_IR_Add);
        REQUIRE(prog.nodes.at(24)->get_microop() == LLVM_IR_Store);
        REQUIRE(prog.nodes.at(46)->get_microop() == LLVM_IR_Load);
        REQUIRE(prog.nodes.at(47)->get_microop() == LLVM_IR_Add);
        REQUIRE(prog.nodes.at(48)->get_microop() == LLVM_IR_Store);

        REQUIRE(prog.getEdgeWeight(10, 11) == 1);
        REQUIRE(prog.getEdgeWeight(11, 12) == 1);
        REQUIRE(prog.getEdgeWeight(22, 23) == 1);
        REQUIRE(prog.getEdgeWeight(23, 24) == 1);
        REQUIRE(prog.getEdgeWeight(46, 47) == 1);
        REQUIRE(prog.getEdgeWeight(47, 48) == 1);
      }
    }

    acc->fuseRegLoadStores();

    WHEN("After applying register load/store fusion") {
      THEN("Edge weights should be set to REGISTER_EDGE") {
        REQUIRE(prog.getEdgeWeight(10, 11) == REGISTER_EDGE);
        REQUIRE(prog.getEdgeWeight(11, 12) == REGISTER_EDGE);
        REQUIRE(prog.getEdgeWeight(22, 23) == REGISTER_EDGE);
        REQUIRE(prog.getEdgeWeight(23, 24) == REGISTER_EDGE);
        REQUIRE(prog.getEdgeWeight(46, 47) == REGISTER_EDGE);
        REQUIRE(prog.getEdgeWeight(47, 48) == REGISTER_EDGE);
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
        unsigned node_22_time =
            prog.nodes.at(22)->get_complete_execution_cycle();
        REQUIRE(prog.nodes.at(23)->get_complete_execution_cycle() ==
                node_22_time + 1);
        REQUIRE(prog.nodes.at(24)->get_complete_execution_cycle() ==
                node_22_time + 1);
        unsigned node_46_time =
            prog.nodes.at(46)->get_complete_execution_cycle();
        REQUIRE(prog.nodes.at(47)->get_complete_execution_cycle() ==
                node_46_time + 1);
        REQUIRE(prog.nodes.at(48)->get_complete_execution_cycle() ==
                node_46_time + 1);
      }
    }
  }
}
