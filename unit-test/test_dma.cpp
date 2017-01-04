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

SCENARIO("Test double buffering memcpy", "[double-buffering]") {
  GIVEN("A simple, single-iteration of a double buffered memcpy, "
        "input size 128 integers, with unrolling factor 2 and "
        "partitioning factor 1") {
    std::string bench("outputs/double-buffering");
    std::string trace_file("inputs/double_buffering_trace.gz");
    std::string config_file("inputs/double_buffering.cfg");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();

    // Location of important nodes.
    unsigned dmaLoad1 = 1;
    unsigned dmaLoad2 = 2;
    unsigned dmaLoad3 = 527;
    unsigned dmaLoad4 = 1048;
    unsigned dmaStore1 = 520;
    unsigned dmaStore2 = 1043;
    unsigned dmaStore3 = 1569;
    unsigned dmaStore4 = 2087;
    unsigned dmaFence1 = 524;
    unsigned dmaFence2 = 1045;
    unsigned firstLoopLoad = 9;
    unsigned secondLoopLoad = 531;
    unsigned firstLoopStore = 11;
    unsigned secondLoopStore = 533;

    WHEN("After building the initial DDDG") {
      THEN("The first two DMA ops are only dependent on memory operations and "
           "fences.") {
        REQUIRE(acc->getNumOfConnectedNodes(dmaLoad1) == 65);
        REQUIRE(acc->getNumOfConnectedNodes(dmaLoad2) == 65);

        THEN("The following DMA loads and stores are also dependent on index "
             "computations.") {
          acc->dumpGraph("double_buffering");
          // First dmaStore is connected to ONE fence (which immediately
          // precedes it) and ONE index computation.
          REQUIRE(acc->getNumOfConnectedNodes(dmaStore1) == 66);
          REQUIRE(acc->doesEdgeExist(dmaStore1 - 1, dmaStore1));
          REQUIRE(acc->doesEdgeExist(dmaStore1, dmaFence1));
          // Second dmaStore is connected to TWO fences and ONE index
          // computation.
          REQUIRE(acc->getNumOfConnectedNodes(dmaStore2) == 67);
          REQUIRE(acc->doesEdgeExist(dmaStore2 - 1, dmaStore2));
          REQUIRE(acc->doesEdgeExist(dmaFence1, dmaStore2));
          REQUIRE(acc->doesEdgeExist(dmaStore2, dmaFence2));
        }
      }
      THEN("None of the DMA ops are isolated.") {
        REQUIRE(acc->getNumOfConnectedNodes(dmaLoad4) > 0);
        REQUIRE(acc->getNumOfConnectedNodes(dmaStore2) > 0);
        REQUIRE(acc->getNumOfConnectedNodes(dmaStore3) > 0);
        REQUIRE(acc->getNumOfConnectedNodes(dmaStore4) > 0);
      }
      THEN("The loads in the memcpy are only connected to dmaLoads, GEPs, "
           "and stores.") {
        for (unsigned node_id = firstLoopLoad; node_id < dmaStore1;
             node_id += 8) {
          REQUIRE(acc->getNumOfConnectedNodes(node_id) == 3);
          REQUIRE(acc->doesEdgeExist(dmaLoad1, node_id) == true);
          REQUIRE(acc->doesEdgeExist(dmaLoad2, node_id) == false);
        }
        for (unsigned node_id = secondLoopLoad; node_id < dmaStore2;
             node_id += 8) {
          REQUIRE(acc->getNumOfConnectedNodes(node_id) == 3);
          REQUIRE(acc->doesEdgeExist(dmaLoad2, node_id) == true);
          REQUIRE(acc->doesEdgeExist(dmaLoad1, node_id) == false);
          REQUIRE(acc->doesEdgeExist(node_id, dmaStore1) == false);
        }
      }
      THEN("The stores in the memcpy are only connected to dmaStores, GEPs, "
           "and loads.") {
        for (unsigned node_id = firstLoopStore; node_id < dmaStore1;
             node_id += 8) {
          REQUIRE(acc->getNumOfConnectedNodes(node_id) == 3);
          REQUIRE(acc->doesEdgeExist(node_id, dmaStore1) == true);
          REQUIRE(acc->doesEdgeExist(node_id, dmaStore2) == false);
          REQUIRE(acc->doesEdgeExist(dmaLoad1, node_id) == false);
          REQUIRE(acc->doesEdgeExist(dmaLoad2, node_id) == false);
        }
        for (unsigned node_id = secondLoopStore; node_id < dmaStore2;
             node_id += 8) {
          REQUIRE(acc->getNumOfConnectedNodes(node_id) == 3);
          REQUIRE(acc->doesEdgeExist(node_id, dmaStore2) == true);
          REQUIRE(acc->doesEdgeExist(dmaLoad1, node_id) == false);
          REQUIRE(acc->doesEdgeExist(dmaLoad2, node_id) == false);
          REQUIRE(acc->doesEdgeExist(node_id, 517) == false);
        }
      }
    }
    WHEN("After applying loop unrolling") {
      acc->loopUnrolling();
      THEN("The number of connected nodes to DMA loads/stores should not "
           "change.") {
        REQUIRE(acc->getNumOfConnectedNodes(dmaLoad1) == 65);
        REQUIRE(acc->getNumOfConnectedNodes(dmaLoad2) == 65);
        REQUIRE(acc->getNumOfConnectedNodes(dmaStore1) == 66);
        REQUIRE(acc->getNumOfConnectedNodes(dmaStore2) == 67);
      }
      THEN(
          "The loads in the memcpy are connected to two more branch nodes.") {
        for (unsigned node_id = firstLoopLoad; node_id < dmaStore1;
             node_id += 8)
          REQUIRE(acc->getNumOfConnectedNodes(node_id) == 5);
        for (unsigned node_id = secondLoopLoad; node_id < dmaStore2;
             node_id += 8)
          REQUIRE(acc->getNumOfConnectedNodes(node_id) == 5);
      }
      THEN("The stores in the memcpy are also connected to two more "
           "branches.") {
        for (unsigned node_id = firstLoopStore; node_id < dmaStore1;
             node_id += 8)
          REQUIRE(acc->getNumOfConnectedNodes(node_id) == 5);
        for (unsigned node_id = secondLoopStore; node_id < dmaStore2;
             node_id += 8)
          REQUIRE(acc->getNumOfConnectedNodes(node_id) == 5);
      }
    }
  }
}
