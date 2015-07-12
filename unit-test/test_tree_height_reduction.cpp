#include "unit-test-config.h"

SCENARIO("Test treeHeightReduction w/ Reduction", "[reduction]") {
  GIVEN("Test Reduction w/ Input Size 128") {
    std::string bench("outputs/reduction-128");
    std::string trace_file("inputs/reduction-128-trace.gz");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    acc->loopUnrolling();
    WHEN("before treeHeightReduction()") {
      THEN("Distance between the last sums in two unrolled iterations should "
           "be 4.") {
        REQUIRE(acc->shortestDistanceBetweenNodes(29, 61) == 4);
        REQUIRE(acc->shortestDistanceBetweenNodes(61, 93) == 4);
        REQUIRE(acc->shortestDistanceBetweenNodes(989, 1021) == 4);
      }
    }
    WHEN("Test treeHeightReduction()") {
      acc->treeHeightReduction();
      THEN("Distance between the last sums in two unrolled iterations should "
           "be 1.") {
        REQUIRE(acc->shortestDistanceBetweenNodes(29, 61) == 1);
        REQUIRE(acc->shortestDistanceBetweenNodes(61, 93) == 1);
        REQUIRE(acc->shortestDistanceBetweenNodes(989, 1021) == 1);
      }
    }
  }
}
