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
    auto& prog = acc->getProgram();
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    acc->loopFlatten();
    acc->loopUnrolling();
    WHEN("before storeBuffer()") {
      THEN("Loads are dependent on previous stores w/ the same addresses.") {
        REQUIRE(prog.edgeExists(15, 23) == 1);
        REQUIRE(prog.edgeExists(27, 35) == 1);
        REQUIRE(prog.edgeExists(1491, 1498) == 1);
      }
      THEN("Loads have children.") {
        REQUIRE(prog.edgeExists(23, 26) == 1);
        REQUIRE(prog.edgeExists(35, 38) == 1);
        REQUIRE(prog.edgeExists(1498, 1506) == 1);
      }
    }
    WHEN("Test storeBuffer()") {
      acc->storeBuffer();
      THEN("Loads are isolated.") {
        REQUIRE(prog.getNumConnectedNodes(23) == 0);
        REQUIRE(prog.getNumConnectedNodes(35) == 0);
        REQUIRE(prog.getNumConnectedNodes(1498) == 0);
      }
      THEN("Loads' children are now store parents' children.") {
        REQUIRE(prog.edgeExists(14, 26) == 1);
        REQUIRE(prog.edgeExists(26, 38) == 1);
        REQUIRE(prog.edgeExists(1489, 1506) == 1);
      }
    }
  }
}
