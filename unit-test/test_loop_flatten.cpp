#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test loopFlatten w/ pp_scan", "[pp_scan]") {
  GIVEN("Test pp_scan w/ Input Size 128, cyclic partition with a factor of 4, "
        "loop unrolling with a factor of 4, enable loop pipelining") {
    std::string bench("outputs/pp_scan-128");
    std::string trace_file("inputs/pp_scan-128-trace.gz");
    std::string config_file("inputs/config-pp_scan-p4-u4-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    const Program& prog = acc->getProgram();
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    WHEN("Test loopFlatten()") {
      acc->loopFlatten();
      THEN("Loop increments become LLVM_IR_Move.") {
        /*
        REQUIRE(prog.nodes.at(16)->get_microop() == LLVM_IR_Move);
        REQUIRE(prog.nodes.at(28)->get_microop() == LLVM_IR_Move);
        REQUIRE(prog.nodes.at(40)->get_microop() == LLVM_IR_Move);
        */
      }
      THEN("Branch nodes for flatten loops are isolated.") {
        REQUIRE(prog.getNumConnectedNodes(20) == 0);
        REQUIRE(prog.getNumConnectedNodes(30) == 0);
      }
    }
  }
}
