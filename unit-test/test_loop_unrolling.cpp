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
        REQUIRE(prog.loop_bounds.at(1).node_id == 30);
        REQUIRE(prog.loop_bounds.at(2).node_id == 54);
        REQUIRE(prog.loop_bounds.at(3).node_id == 78);
        REQUIRE(prog.loop_bounds.at(21).node_id == 510);
        REQUIRE(prog.loop_bounds.at(64).node_id == 1542);
      }
      THEN("Branch nodes inside unrolled iterations are isolated.") {
        REQUIRE(prog.getNumConnectedNodes(18) == 0);
        REQUIRE(prog.getNumConnectedNodes(30) != 0);
        REQUIRE(prog.getNumConnectedNodes(42) == 0);
        REQUIRE(prog.getNumConnectedNodes(1530) == 0);
        REQUIRE(prog.getNumConnectedNodes(1542) != 0);
      }
      THEN("Branch edges are added between boundary branch nodes and nodes "
           "in the next unrolled iterations.") {
        REQUIRE(prog.edgeExists(6, 20));
        REQUIRE(prog.edgeExists(30, 44));
        REQUIRE(prog.edgeExists(54, 68));
        REQUIRE(prog.edgeExists(78, 92));
        REQUIRE(prog.edgeExists(510, 524));
        REQUIRE(prog.edgeExists(1518, 1532));
      }
      THEN("Branch edges are added between boundary nodes and nodes inside "
           "the current unrolled iterations.") {
        REQUIRE(prog.edgeExists(6, 16));
        REQUIRE(prog.edgeExists(30, 40));
        REQUIRE(prog.edgeExists(54, 64));
        REQUIRE(prog.edgeExists(510, 520));
        REQUIRE(prog.edgeExists(1518, 1528));
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
        REQUIRE(prog.loop_bounds.at(1).node_id == 35);
        REQUIRE(prog.loop_bounds.at(2).node_id == 67);
        REQUIRE(prog.loop_bounds.at(32).node_id == 1027);
      }
      THEN("Branch nodes inside unrolled iterations are isolated.") {
        REQUIRE(prog.getNumConnectedNodes(11) == 0);
        REQUIRE(prog.getNumConnectedNodes(19) == 0);
        REQUIRE(prog.getNumConnectedNodes(27) == 0);
        REQUIRE(prog.getNumConnectedNodes(35) != 0);
        REQUIRE(prog.getNumConnectedNodes(1003) == 0);
        REQUIRE(prog.getNumConnectedNodes(1011) == 0);
        REQUIRE(prog.getNumConnectedNodes(1019) == 0);
        REQUIRE(prog.getNumConnectedNodes(1027) != 0);
      }
      THEN("Branch edges are added between boundary branch nodes and nodes "
           "in the next unrolled iterations.") {
        REQUIRE(prog.edgeExists(3, 14));
        REQUIRE(prog.edgeExists(35, 46));
        REQUIRE(prog.edgeExists(67, 78));
        REQUIRE(prog.edgeExists(995, 1006));
      }
      THEN("Branch edges are added between boundary nodes and nodes inside "
           "the current unrolled iterations.") {
        REQUIRE(prog.edgeExists(3, 6));
        REQUIRE(prog.edgeExists(35, 38));
        REQUIRE(prog.edgeExists(995, 998));
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
        // Call break to local_scan.
        REQUIRE(prog.loop_bounds.at(0).node_id == 3);
        REQUIRE(prog.loop_bounds.at(1).node_id == 4);
        REQUIRE(prog.loop_bounds.at(2).node_id == 640);
        REQUIRE(prog.loop_bounds.at(3).node_id == 1276);
        // Call break to sum_scan.
        REQUIRE(prog.loop_bounds.at(4).node_id == 1278);
        REQUIRE(prog.loop_bounds.at(5).node_id == 1280);
        REQUIRE(prog.loop_bounds.at(6).node_id == 1328);
        REQUIRE(prog.loop_bounds.at(7).node_id == 1364);
        // Call break to last_step_scan.
        REQUIRE(prog.loop_bounds.at(8).node_id == 1366);
        REQUIRE(prog.loop_bounds.at(9).node_id == 1367);
        REQUIRE(prog.loop_bounds.at(10).node_id == 2099);
        REQUIRE(prog.loop_bounds.at(11).node_id == 2831);
        // Exit.
        REQUIRE(prog.loop_bounds.at(12).node_id == 2837);
      }
      THEN("Branch nodes inside unrolled iterations are isolated.") {
        REQUIRE(prog.getNumConnectedNodes(163) == 0);
        REQUIRE(prog.getNumConnectedNodes(1292) == 0);
        REQUIRE(prog.getNumConnectedNodes(1550) == 0);
      }
      THEN("Branch edges are added between boundary branch nodes and nodes "
           "in the next unrolled iterations.") {
        REQUIRE(prog.edgeExists(4, 165));
        REQUIRE(prog.edgeExists(1280, 1295));
        REQUIRE(prog.edgeExists(1367, 1552));
      }
      THEN("Branch edges are added between boundary nodes and nodes inside "
           "the current unrolled iterations.") {
        REQUIRE(prog.edgeExists(4, 6));
        REQUIRE(prog.edgeExists(1280, 1283));
        REQUIRE(prog.edgeExists(1367, 1369));
      }
    }
  }
}
