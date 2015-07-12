#include "unit-test-config.h"

SCENARIO("Test loopFlatten w/ pp_scan", "[pp_scan]") {
  GIVEN("Test pp_scan w/ Input Size 128, cyclic partition with a factor of 4, "
        "loop unrolling with a factor of 4, enable loop pipelining") {
    std::string bench("outputs/pp_scan-128");
    std::string trace_file("inputs/pp_scan-128-trace.gz");
    std::string config_file("inputs/config-pp_scan-p4-u4-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    WHEN("Test loopFlatten()") {
      acc->loopFlatten();
      THEN("Loop increments become LLVM_IR_Move.") {
        REQUIRE(acc->getMicroop(16) == LLVM_IR_Move);
        REQUIRE(acc->getMicroop(28) == LLVM_IR_Move);
        REQUIRE(acc->getMicroop(40) == LLVM_IR_Move);
      }
      THEN("Branch nodes for flatten loops are isolated.") {
        REQUIRE(acc->getNumOfConnectedNodes(18) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(30) == 0);
      }
    }
  }
}
