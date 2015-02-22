#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test loopUnrolling w/ Triad", "[triad]")
{
  GIVEN("Test Triad w/ Input Size 128, cyclic partition with a factor of 2, "
        "loop unrolling with a factor of 2, enable loop pipelining")
  {
    std::string bench("outputs/triad-128");
    std::string trace_file("inputs/triad-128-trace");
    std::string config_file("inputs/config-triad-p2-u2-P1");

    ScratchpadDatapath *acc;
    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    WHEN("Test loopUnrolling()")
    {
      acc->loopUnrolling();
      THEN("Unrolled loop boundary should match the expectations.")
      {
        REQUIRE(acc->getUnrolledLoopBoundary(1) == 24);
        REQUIRE(acc->getUnrolledLoopBoundary(2) == 48);
        REQUIRE(acc->getUnrolledLoopBoundary(3) == 72);
        REQUIRE(acc->getUnrolledLoopBoundary(21) == 504);
        REQUIRE(acc->getUnrolledLoopBoundary(64) == 1536);
      }
      THEN("Branch nodes inside unrolled iterations are isolated.")
      {
        REQUIRE(acc->getNumOfConnectedNodes(12) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(24) != 0);
        REQUIRE(acc->getNumOfConnectedNodes(36) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1524) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1536) != 0);
      }
      THEN("Branch edges are added between boundary branch nodes and nodes "
           "in the next unrolled iterations.")
      {
        REQUIRE(acc->doesEdgeExist(2, 9) == 1);
        REQUIRE(acc->doesEdgeExist(24, 27) == 1);
        REQUIRE(acc->doesEdgeExist(48, 53) == 1);
        REQUIRE(acc->doesEdgeExist(72, 81) == 1);
        REQUIRE(acc->doesEdgeExist(504, 513) == 1);
        REQUIRE(acc->doesEdgeExist(1512, 1527) == 1);
      }
      THEN("Branch edges are added between boundary nodes and nodes inside "
           "the current unrolled iterations.")
      {
        REQUIRE(acc->doesEdgeExist(3, 24) == 1);
        REQUIRE(acc->doesEdgeExist(27, 48) == 1);
        REQUIRE(acc->doesEdgeExist(53, 72) == 1);
        REQUIRE(acc->doesEdgeExist(501, 504) == 1);
        REQUIRE(acc->doesEdgeExist(1503, 1512) == 1);
      }
    }
  }
}
SCENARIO("Test loopUnrolling w/ Reduction", "[reduction]")
{
  GIVEN("Test Reduction w/ Input Size 128, cyclic partition with a factor of 4, "
        "loop unrolling with a factor of 4, enable loop pipelining")
  {
    std::string bench("outputs/reduction-128");
    std::string trace_file("inputs/reduction-128-trace");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

    ScratchpadDatapath *acc;
    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    WHEN("Test loopUnrolling()")
    {
      acc->loopUnrolling();
      THEN("Unrolled loop boundary should match the expectations.")
      {
        REQUIRE(acc->getUnrolledLoopBoundary(1) == 32);
        REQUIRE(acc->getUnrolledLoopBoundary(2) == 64);
        REQUIRE(acc->getUnrolledLoopBoundary(32) == 1024);
      }
      THEN("Branch nodes inside unrolled iterations are isolated.")
      {
        REQUIRE(acc->getNumOfConnectedNodes(8) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(24) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(32) != 0);
        REQUIRE(acc->getNumOfConnectedNodes(1016) == 0);
      }
      THEN("Branch edges are added between boundary branch nodes and nodes "
           "in the next unrolled iterations.")
      {
        REQUIRE(acc->doesEdgeExist(3, 5) == 1);
        REQUIRE(acc->doesEdgeExist(32, 37) == 1);
        REQUIRE(acc->doesEdgeExist(32, 45) == 1);
        REQUIRE(acc->doesEdgeExist(992, 1021) == 1);
      }
      THEN("Branch edges are added between boundary nodes and nodes inside "
           "the current unrolled iterations.")
      {
        REQUIRE(acc->doesEdgeExist(5, 32) == 1);
        REQUIRE(acc->doesEdgeExist(29, 32) == 1);
        REQUIRE(acc->doesEdgeExist(1021, 1024) == 1);
      }
    }
  }
}
SCENARIO("Test loopUnrolling w/ pp_scan", "[pp_scan]")
{
  GIVEN("Test pp_scan w/ Input Size 128, cyclic partition with a factor of 4, "
        "loop unrolling with a factor of 4, enable loop pipelining")
  {
    std::string bench("outputs/pp_scan-128");
    std::string trace_file("inputs/pp_scan-128-trace");
    std::string config_file("inputs/config-pp_scan-p4-u4-P1");

    ScratchpadDatapath *acc;
    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    acc->loopFlatten();
    WHEN("Test loopUnrolling()")
    {
      acc->loopUnrolling();
      THEN("Unrolled loop boundary should match the expectations.")
      {
        REQUIRE(acc->getUnrolledLoopBoundary(0) == 0);
        REQUIRE(acc->getUnrolledLoopBoundary(1) == 369);
        REQUIRE(acc->getUnrolledLoopBoundary(2) == 737);
        REQUIRE(acc->getUnrolledLoopBoundary(3) == 1105);
        REQUIRE(acc->getUnrolledLoopBoundary(4) == 1473);
        REQUIRE(acc->getUnrolledLoopBoundary(5) == 1475); //call break
        REQUIRE(acc->getUnrolledLoopBoundary(6) == 1545);
        REQUIRE(acc->getUnrolledLoopBoundary(7) == 1613);
        REQUIRE(acc->getUnrolledLoopBoundary(8) == 1681);
        REQUIRE(acc->getUnrolledLoopBoundary(9) == 1734); //call break
        REQUIRE(acc->getUnrolledLoopBoundary(10) == 2095);
        REQUIRE(acc->getUnrolledLoopBoundary(11) == 2455);
        REQUIRE(acc->getUnrolledLoopBoundary(12) == 2815);
        REQUIRE(acc->getUnrolledLoopBoundary(13) == 3175);
      }
      THEN("Branch nodes inside unrolled iterations are isolated.")
      {
        REQUIRE(acc->getNumOfConnectedNodes(93) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1511) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1545) != 0);
        REQUIRE(acc->getNumOfConnectedNodes(3085) == 0);
      }
      THEN("Branch edges are added between boundary branch nodes and nodes "
          "in the next unrolled iterations.")
      {
        REQUIRE(acc->doesEdgeExist(0, 11) == 1);
        REQUIRE(acc->doesEdgeExist(369, 379) == 1);
        REQUIRE(acc->doesEdgeExist(1734, 1746) == 1);
        REQUIRE(acc->doesEdgeExist(2455, 2466) == 1);
      }
      THEN("Branch edges are added between boundary nodes and nodes inside "
          "the current unrolled iterations.")
      {
        REQUIRE(acc->doesEdgeExist(11, 369) == 1);
        REQUIRE(acc->doesEdgeExist(379, 737) == 1);
        REQUIRE(acc->doesEdgeExist(1726, 1734) == 1);
        REQUIRE(acc->doesEdgeExist(2447, 2455) == 1);
      }
    }
  }
}
