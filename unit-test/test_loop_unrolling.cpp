#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test loopUnrolling w/ Triad", "[triad]") {
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
    WHEN("Test loopUnrolling()") {
      acc->loopUnrolling();
      THEN("Unrolled loop boundary should match the expectations.") {
        REQUIRE(prog.loop_bounds.at(1).node_id == 24);
        REQUIRE(prog.loop_bounds.at(2).node_id == 48);
        REQUIRE(prog.loop_bounds.at(3).node_id == 72);
        REQUIRE(prog.loop_bounds.at(21).node_id == 504);
        REQUIRE(prog.loop_bounds.at(64).node_id == 1536);
      }
      THEN("Branch nodes inside unrolled iterations are isolated.") {
        REQUIRE(prog.getNumConnectedNodes(12) == 0);
        REQUIRE(prog.getNumConnectedNodes(24) != 0);
        REQUIRE(prog.getNumConnectedNodes(36) == 0);
        REQUIRE(prog.getNumConnectedNodes(1524) == 0);
        REQUIRE(prog.getNumConnectedNodes(1536) != 0);
      }
      THEN("Branch edges are added between boundary branch nodes and nodes "
           "in the next unrolled iterations.") {
        REQUIRE(prog.edgeExists(0, 9));
        REQUIRE(prog.edgeExists(24, 27));
        REQUIRE(prog.edgeExists(48, 53));
        REQUIRE(prog.edgeExists(72, 81));
        REQUIRE(prog.edgeExists(504, 513));
        REQUIRE(prog.edgeExists(1512, 1527));
      }
      THEN("Branch edges are added between boundary nodes and nodes inside "
           "the current unrolled iterations.") {
        REQUIRE(prog.edgeExists(3, 24));
        REQUIRE(prog.edgeExists(27, 48));
        REQUIRE(prog.edgeExists(53, 72));
        REQUIRE(prog.edgeExists(501, 504));
        REQUIRE(prog.edgeExists(1503, 1512));
      }
    }
  }
}
SCENARIO("Test loopUnrolling w/ Reduction", "[reduction]") {
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
    WHEN("Test loopUnrolling()") {
      acc->loopUnrolling();
      THEN("Unrolled loop boundary should match the expectations.") {
        REQUIRE(prog.loop_bounds.at(1).node_id == 32);
        REQUIRE(prog.loop_bounds.at(2).node_id == 64);
        REQUIRE(prog.loop_bounds.at(32).node_id == 1024);
      }
      THEN("Branch nodes inside unrolled iterations are isolated.") {
        REQUIRE(prog.getNumConnectedNodes(8) == 0);
        REQUIRE(prog.getNumConnectedNodes(24) == 0);
        REQUIRE(prog.getNumConnectedNodes(32) != 0);
        REQUIRE(prog.getNumConnectedNodes(1016) == 0);
      }
      THEN("Branch edges are added between boundary branch nodes and nodes "
           "in the next unrolled iterations.") {
        REQUIRE(prog.edgeExists(0, 5));
        REQUIRE(prog.edgeExists(32, 37));
        REQUIRE(prog.edgeExists(32, 45));
        REQUIRE(prog.edgeExists(992, 1021));
      }
      THEN("Branch edges are added between boundary nodes and nodes inside "
           "the current unrolled iterations.") {
        REQUIRE(prog.edgeExists(5, 32));
        REQUIRE(prog.edgeExists(29, 32));
        REQUIRE(prog.edgeExists(1021, 1024));
      }
    }
  }
}
SCENARIO("Test loopUnrolling w/ pp_scan", "[pp_scan]") {
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
    WHEN("Test loopUnrolling()") {
      acc->loopUnrolling();
      THEN("Unrolled loop boundary should match the expectations.") {
        REQUIRE(prog.loop_bounds.at(0).node_id == 0); // call break
        REQUIRE(prog.loop_bounds.at(1).node_id == 1);
        REQUIRE(prog.loop_bounds.at(2).node_id == 369);
        REQUIRE(prog.loop_bounds.at(3).node_id == 737);
        REQUIRE(prog.loop_bounds.at(4).node_id == 1105);
        REQUIRE(prog.loop_bounds.at(5).node_id == 1473);
        REQUIRE(prog.loop_bounds.at(6).node_id == 1475);  // call break
        REQUIRE(prog.loop_bounds.at(7).node_id == 1477);
        REQUIRE(prog.loop_bounds.at(8).node_id == 1545);
        REQUIRE(prog.loop_bounds.at(9).node_id == 1613);
        REQUIRE(prog.loop_bounds.at(10).node_id == 1681);
        REQUIRE(prog.loop_bounds.at(11).node_id == 1732);
        REQUIRE(prog.loop_bounds.at(12).node_id == 1734);  // call break
        REQUIRE(prog.loop_bounds.at(13).node_id == 1735);
        REQUIRE(prog.loop_bounds.at(14).node_id == 2095);
        REQUIRE(prog.loop_bounds.at(15).node_id == 2455);
        REQUIRE(prog.loop_bounds.at(16).node_id == 2815);
        REQUIRE(prog.loop_bounds.at(17).node_id == 3175);
        REQUIRE(prog.loop_bounds.at(18).node_id == 3178);  // end
      }
      THEN("Branch nodes inside unrolled iterations are isolated.") {
        REQUIRE(prog.getNumConnectedNodes(93) == 0);
        REQUIRE(prog.getNumConnectedNodes(1511) == 0);
        REQUIRE(prog.getNumConnectedNodes(1545) != 0);
        REQUIRE(prog.getNumConnectedNodes(3085) == 0);
      }
      THEN("Branch edges are added between boundary branch nodes and nodes "
           "in the next unrolled iterations.") {
        REQUIRE(prog.edgeExists(1, 11));
        REQUIRE(prog.edgeExists(369, 379));
        REQUIRE(prog.edgeExists(1735, 1746));
        REQUIRE(prog.edgeExists(2455, 2466));
      }
      THEN("Branch edges are added between boundary nodes and nodes inside "
           "the current unrolled iterations.") {
        REQUIRE(prog.edgeExists(11, 369));
        REQUIRE(prog.edgeExists(379, 737));
        REQUIRE(prog.edgeExists(1726, 1732));
        REQUIRE(prog.edgeExists(2447, 2455));
      }
    }
  }
}
