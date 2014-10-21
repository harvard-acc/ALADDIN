#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test loopPipelining w/ Triad", "[triad]")
{
  GIVEN("Test Triad w/ Input Size 128, cyclic partition with a factor of 2, "
        "loop unrolling with a factor of 2, enable loop pipelining")
  {
    std::string bench("triad-128");
    std::string trace_file("inputs/triad-128-trace");
    std::string config_file("inputs/config-triad-p2-u2-P1");

    ScratchpadDatapath *acc;
    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
    spad = new Scratchpad(1);
    acc->setScratchpad(spad);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    acc->loopUnrolling();
    WHEN("Test loopPipelining()")
    {
      acc->loopPipelining();
      THEN("Dependences exist between first instructions of pipelined iterations.")
      {
        REQUIRE(acc->doesEdgeExist(2, 26) == 1);
        REQUIRE(acc->doesEdgeExist(26, 50) == 1);
        REQUIRE(acc->doesEdgeExist(50, 74) == 1);
        REQUIRE(acc->doesEdgeExist(482, 506) == 1);
        REQUIRE(acc->doesEdgeExist(1490, 1514) == 1);
      }
      THEN("Dependences exist between the first instructions and the rest of "
           "instructions in the same loop region.")
      {
        REQUIRE(acc->doesEdgeExist(2, 9) == 1);
        REQUIRE(acc->doesEdgeExist(26, 29) == 1);
        REQUIRE(acc->doesEdgeExist(26, 33) == 1);
        REQUIRE(acc->doesEdgeExist(50, 54) == 1);
        REQUIRE(acc->doesEdgeExist(482, 487) == 1);
        REQUIRE(acc->doesEdgeExist(1490, 1505) == 1);
      }
      THEN("Branch edges are removed between boundary branch nodes and nodes "
           "in the next unrolled iterations.")
      {
        REQUIRE(acc->doesEdgeExist(24, 27) == 0);
        REQUIRE(acc->doesEdgeExist(48, 53) == 0);
        REQUIRE(acc->doesEdgeExist(72, 81) == 0);
        REQUIRE(acc->doesEdgeExist(504, 513) == 0);
        REQUIRE(acc->doesEdgeExist(1512, 1527) == 0);
      }
    }
  }
}
SCENARIO("Test loopPipelining w/ Reduction", "[reduction]")
{
  GIVEN("Test Reduction w/ Input Size 128, cyclic partition with a factor of 4, "
        "loop unrolling with a factor of 4, enable loop pipelining")
  {
    std::string bench("reduction-128");
    std::string trace_file("inputs/reduction-128-trace");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

    ScratchpadDatapath *acc;
    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
    spad = new Scratchpad(1);
    acc->setScratchpad(spad);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    acc->loopUnrolling();
    acc->treeHeightReduction();
    WHEN("Test loopPipelining()")
    {
      acc->loopPipelining();
      THEN("Dependences exist between first instructions of pipelined iterations.")
      {
        REQUIRE(acc->doesEdgeExist(3, 35) == 1);
        REQUIRE(acc->doesEdgeExist(35, 67) == 1);
        REQUIRE(acc->doesEdgeExist(963, 995) == 1);
      }
      THEN("Dependences exist between the first instructions and the rest of "
          "instructions in the same loop region.")
      {
        REQUIRE(acc->doesEdgeExist(3, 11) == 1);
        REQUIRE(acc->doesEdgeExist(35, 53) == 1);
        REQUIRE(acc->doesEdgeExist(995, 1012) == 1);
      }
      THEN("Branch edges are removed between boundary branch nodes and nodes "
          "in the next unrolled iterations.")
      {
        REQUIRE(acc->doesEdgeExist(32, 37) == 0);
        REQUIRE(acc->doesEdgeExist(32, 45) == 0);
        REQUIRE(acc->doesEdgeExist(992, 1021) == 0);
      }
    }
  }
}
