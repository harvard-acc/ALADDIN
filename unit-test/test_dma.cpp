#include "catch.hpp"
#include <iostream>
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
      THEN("Before loop unrolling, all DMA ops are only dependent on memory operations.") {
        REQUIRE(acc->getNumOfConnectedNodes(0) == 2048);  // dmaLoad
        REQUIRE(acc->getNumOfConnectedNodes(1) == 2048);  // dmaLoad
        REQUIRE(acc->getNumOfConnectedNodes(24579) == 2048);  // dmaStore
      }
      THEN("Before loop unrolling, the last dmaStore cannot proceed until all stores "
           "to the array has completed.") {
        REQUIRE(acc->getNumOfConnectedNodes(24579) == 2048);
      }
      THEN("Before loop unrolling, there is no edge between the first two dmaLoads.") {
        REQUIRE(acc->doesEdgeExist(0, 1) == false);
      }
      THEN("Before loop unrolling, there is no edge between the dmaStore and the "
           "previous boundary node.") {
        REQUIRE(acc->doesEdgeExist(24578, 24579) == false);
      }
    }
    WHEN("Test DMA dependence after loop unrolling.") {
      acc->loopUnrolling();
      THEN("First, the number of dependences on the DMA operations do not change. ") {
        REQUIRE(acc->getNumOfConnectedNodes(0) == 2048);
        REQUIRE(acc->getNumOfConnectedNodes(1) == 2048);
        REQUIRE(acc->getNumOfConnectedNodes(24579) == 2048);
      }
      THEN("Second, there is no edge between the first two dmaLoads.") {
        REQUIRE(acc->doesEdgeExist(0, 1) == false);
      }
      THEN("Third, the boundary node after the two dmaLoads is independent of "
           "both of them, because the dmaLoads will only block memory nodes.") {
        REQUIRE(acc->doesEdgeExist(0, 2) == false);
        REQUIRE(acc->doesEdgeExist(1, 2) == false);
      }
      THEN("Fourth, the dmaStore remains independent of the previous boundary node."){
        REQUIRE(acc->doesEdgeExist(24578, 24579) == false);
      }
    }
  }
}
