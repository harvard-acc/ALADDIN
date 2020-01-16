#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test treeHeightReduction w/ Reduction", "[reduction]") {
  GIVEN("Test Reduction w/ Input Size 128") {
    std::string bench("outputs/reduction-128");
    std::string trace_file("inputs/reduction-128-trace.gz");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    auto& prog = acc->getProgram();
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    acc->loopUnrolling();
    WHEN("before treeHeightReduction()") {
      THEN("Distance between the last sums in two unrolled iterations should "
           "be 4.") {
        REQUIRE(prog.shortestDistanceBetweenNodes(32, 64) == 4);
        REQUIRE(prog.shortestDistanceBetweenNodes(64, 96) == 4);
        REQUIRE(prog.shortestDistanceBetweenNodes(992, 1024) == 4);
      }
    }
    WHEN("Test treeHeightReduction()") {
      acc->treeHeightReduction();
      THEN("Distance between the last sums in two unrolled iterations should "
           "be 1.") {
        REQUIRE(prog.shortestDistanceBetweenNodes(32, 64) == 1);
        REQUIRE(prog.shortestDistanceBetweenNodes(64, 96) == 1);
        REQUIRE(prog.shortestDistanceBetweenNodes(992, 1024) == 1);
      }
    }
  }
}
