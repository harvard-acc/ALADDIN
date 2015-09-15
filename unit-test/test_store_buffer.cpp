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
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    acc->loopFlatten();
    acc->loopUnrolling();
    WHEN("before storeBuffer()") {
      THEN("Loads are dependent on previous stores w/ the same addresses.") {
        REQUIRE(acc->doesEdgeExist(15, 23) == 1);
        REQUIRE(acc->doesEdgeExist(27, 35) == 1);
        REQUIRE(acc->doesEdgeExist(1491, 1498) == 1);
      }
      THEN("Loads have children.") {
        REQUIRE(acc->doesEdgeExist(23, 26) == 1);
        REQUIRE(acc->doesEdgeExist(35, 38) == 1);
        REQUIRE(acc->doesEdgeExist(1498, 1506) == 1);
      }
    }
    WHEN("Test storeBuffer()") {
      acc->storeBuffer();
      THEN("Loads are isolated.") {
        REQUIRE(acc->getNumOfConnectedNodes(23) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(35) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1498) == 0);
      }
      THEN("Loads' children are now store parents' children.") {
        REQUIRE(acc->doesEdgeExist(14, 26) == 1);
        REQUIRE(acc->doesEdgeExist(26, 38) == 1);
        REQUIRE(acc->doesEdgeExist(1489, 1506) == 1);
      }
    }
  }
}
