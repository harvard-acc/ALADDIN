#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test loopPipelining w/ Triad", "[triad]") {
  GIVEN("Test Triad w/ Input Size 128, cyclic partition with a factor of 2, "
        "loop unrolling with a factor of 2, enable loop pipelining") {
    std::string bench("outputs/triad-128");
    std::string trace_file("inputs/triad-128-trace.gz");
    std::string config_file("inputs/config-triad-p2-u2-P1");

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
    WHEN("Test loopPipelining()") {
      acc->perLoopPipelining();
      THEN("Dependences exist between first instructions of pipelined "
           "iterations.") {
        REQUIRE(prog.edgeExists(8, 32) == 1);
        REQUIRE(prog.edgeExists(32, 56) == 1);
        REQUIRE(prog.edgeExists(56, 80) == 1);
        REQUIRE(prog.edgeExists(488, 512) == 1);
        REQUIRE(prog.edgeExists(1496, 1520) == 1);
      }
      THEN("Dependences exist between the first instructions and the rest of "
           "instructions in the same loop region.") {
        REQUIRE(prog.edgeExists(6, 8) == 1);
        REQUIRE(prog.edgeExists(32, 33) == 1);
        REQUIRE(prog.edgeExists(56, 57) == 1);
        REQUIRE(prog.edgeExists(488, 489) == 1);
        REQUIRE(prog.edgeExists(1496, 1497) == 1);
      }
      THEN("Branch edges are removed between boundary branch nodes and nodes "
           "in the next unrolled iterations.") {
        REQUIRE(prog.edgeExists(30, 53) == 0);
        REQUIRE(prog.edgeExists(54, 77) == 0);
        REQUIRE(prog.edgeExists(78, 101) == 0);
        REQUIRE(prog.edgeExists(510, 533) == 0);
        REQUIRE(prog.edgeExists(1494, 1517) == 0);
      }
    }
  }
}
SCENARIO("Test loopPipelining w/ Reduction", "[reduction]") {
  GIVEN(
      "Test Reduction w/ Input Size 128, cyclic partition with a factor of 4, "
      "loop unrolling with a factor of 4, enable loop pipelining") {
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
    acc->treeHeightReduction();
    WHEN("Test loopPipelining()") {
      acc->perLoopPipelining();
      THEN("Dependences exist between first instructions of pipelined "
           "iterations.") {
        REQUIRE(prog.edgeExists(6, 38) == 1);
        REQUIRE(prog.edgeExists(38, 70) == 1);
        REQUIRE(prog.edgeExists(966, 998) == 1);
      }
      THEN("Dependences exist between the first instructions and the rest of "
           "instructions in the same loop region.") {
        REQUIRE(prog.edgeExists(3, 18) == 1);
        REQUIRE(prog.edgeExists(38, 54) == 1);
        REQUIRE(prog.edgeExists(998, 1014) == 1);
      }
      THEN("Branch edges are removed between boundary branch nodes and nodes "
           "in the next unrolled iterations.") {
        REQUIRE(prog.edgeExists(35, 37) == 0);
        REQUIRE(prog.edgeExists(35, 45) == 0);
        REQUIRE(prog.edgeExists(995, 1021) == 0);
      }
    }
  }
}
