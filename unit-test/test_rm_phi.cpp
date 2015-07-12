#include "unit-test-config.h"

SCENARIO("Test removePhiNodes w/ Triad", "[triad]") {
  GIVEN("Test Triad w/ Input Size 128") {
    std::string bench("outputs/triad-128");
    std::string trace_file("inputs/triad-128-trace.gz");
    std::string config_file("inputs/config-triad-p2-u2-P1");

    trace_file = root_dir + trace_file;
    config_file = root_dir + config_file;

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    acc->removeInductionDependence();
    WHEN("Test removePhiNodes()") {
      acc->removePhiNodes();
      THEN("Phi Nodes in the DDDG should be isolated.") {
        REQUIRE(acc->getNumOfConnectedNodes(13) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(25) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1525) == 0);
      }
    }
  }
}
SCENARIO("Test removePhiNodes w/ Reduction", "[reduction]") {
  GIVEN("Test Reduction w/ Input Size 128") {
    std::string bench("outputs/reduction-128");
    std::string trace_file("inputs/reduction-128-trace.gz");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

    trace_file = root_dir + trace_file;
    config_file = root_dir + config_file;

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    acc->removeInductionDependence();
    WHEN("Test removePhiNodes()") {
      acc->removePhiNodes();
      THEN("Phi Nodes in the DDDG should be isolated.") {
        REQUIRE(acc->getNumOfConnectedNodes(1) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(10) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1009) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1018) == 0);
      }
    }
  }
}
SCENARIO("Test removePhiNodes w/ Pp_scan", "[pp_scan]") {
  GIVEN("Test Pp_scan w/ Input Size 128") {
    std::string bench("outputs/pp_scan-128");
    std::string trace_file("inputs/pp_scan-128-trace.gz");
    std::string config_file("inputs/config-pp_scan-p4-u4-P1");

    trace_file = root_dir + trace_file;
    config_file = root_dir + config_file;

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    acc->removeInductionDependence();
    WHEN("Test removePhiNodes()") {
      acc->removePhiNodes();
      THEN("Phi Nodes in the DDDG should be isolated.") {
        REQUIRE(acc->getNumOfConnectedNodes(2) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(19) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1478) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1495) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1736) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(3163) == 0);
      }
    }
  }
}
