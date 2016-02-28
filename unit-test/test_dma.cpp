#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test DMA Dependence w/ Triad", "[triad]") {
  GIVEN("Test Triad w/ Input Size 2048, cyclic partition with a factor of 2, "
        "loop unrolling with a factor of 2, enable loop pipelining") {
    std::string bench("outputs/triad-dma");
    std::string trace_file("inputs/triad-dma-trace.gz");
    std::string config_file("inputs/config-triad-dma-p2-u2-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    WHEN("Test DMA dependence before loop unrolling.") {
      THEN("Before loop unrolling, the first two dmaLoads are isolated. ") {
        REQUIRE(acc->getNumOfConnectedNodes(0) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1) == 0);
      }
      THEN("Before loop unrolling, the last dmaStore is isolated. ") {
        REQUIRE(acc->getNumOfConnectedNodes(24579) == 0);
      }
    }
    WHEN("Test DMA dependence after loop unrolling.") {
      acc->loopUnrolling();
      THEN("First, dma operations are not isolated. ") {
        REQUIRE(acc->getNumOfConnectedNodes(0) != 0);
        REQUIRE(acc->getNumOfConnectedNodes(1) != 0);
        REQUIRE(acc->getNumOfConnectedNodes(24579) != 0);
      }
      THEN("Second, there is no edge between the first two dmaLoads.") {
        REQUIRE(acc->doesEdgeExist(0, 1) == false);
      }
      THEN("Third, the boundary node after the two dmaLoads is dependent on both of them.") {
        REQUIRE(acc->doesEdgeExist(0, 2) == true);
        REQUIRE(acc->doesEdgeExist(1, 2) == true);
      }
      THEN("Fourth, dmaStore is dependent of the previous boundary node."){
        REQUIRE(acc->doesEdgeExist(24578, 24579) == true);
      }
    }
  }
}
