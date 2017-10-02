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
      acc->loopPipelining();
      THEN("Dependences exist between first instructions of pipelined "
           "iterations.") {
        REQUIRE(prog.edgeExists(2, 26) == 1);
        REQUIRE(prog.edgeExists(26, 50) == 1);
        REQUIRE(prog.edgeExists(50, 74) == 1);
        REQUIRE(prog.edgeExists(482, 506) == 1);
        REQUIRE(prog.edgeExists(1490, 1514) == 1);
      }
      THEN("Dependences exist between the first instructions and the rest of "
           "instructions in the same loop region.") {
        REQUIRE(prog.edgeExists(0, 9) == 1);
        REQUIRE(prog.edgeExists(26, 29) == 1);
        REQUIRE(prog.edgeExists(26, 33) == 1);
        REQUIRE(prog.edgeExists(50, 54) == 1);
        REQUIRE(prog.edgeExists(482, 487) == 1);
        REQUIRE(prog.edgeExists(1490, 1505) == 1);
      }
      THEN("Branch edges are removed between boundary branch nodes and nodes "
           "in the next unrolled iterations.") {
        REQUIRE(prog.edgeExists(24, 27) == 0);
        REQUIRE(prog.edgeExists(48, 53) == 0);
        REQUIRE(prog.edgeExists(72, 81) == 0);
        REQUIRE(prog.edgeExists(504, 513) == 0);
        REQUIRE(prog.edgeExists(1512, 1527) == 0);
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
      acc->loopPipelining();
      THEN("Dependences exist between first instructions of pipelined "
           "iterations.") {
        REQUIRE(prog.edgeExists(3, 35) == 1);
        REQUIRE(prog.edgeExists(35, 67) == 1);
        REQUIRE(prog.edgeExists(963, 995) == 1);
      }
      THEN("Dependences exist between the first instructions and the rest of "
           "instructions in the same loop region.") {
        REQUIRE(prog.edgeExists(0, 11) == 1);
        REQUIRE(prog.edgeExists(35, 53) == 1);
        REQUIRE(prog.edgeExists(995, 1012) == 1);
      }
      THEN("Branch edges are removed between boundary branch nodes and nodes "
           "in the next unrolled iterations.") {
        REQUIRE(prog.edgeExists(32, 37) == 0);
        REQUIRE(prog.edgeExists(32, 45) == 0);
        REQUIRE(prog.edgeExists(992, 1021) == 0);
      }
    }
  }
}
