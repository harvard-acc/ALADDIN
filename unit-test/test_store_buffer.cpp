#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test store buffering", "[sb]") {
  std::string bench("outputs/store_buffer");
  std::string trace_file("inputs/store_buffer.gz");
  std::string config_file("inputs/config-store-buffer");

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
      REQUIRE(prog.edgeExists(15, 21) == 1);
      REQUIRE(prog.edgeExists(41, 47) == 1);
      REQUIRE(prog.edgeExists(67, 73) == 1);
    }
    THEN("Loads have children.") {
      REQUIRE(prog.edgeExists(21, 26) == 1);
      REQUIRE(prog.edgeExists(47, 52) == 1);
      REQUIRE(prog.edgeExists(73, 78) == 1);
    }
  }
  WHEN("Test storeBuffer()") {
    acc->storeBuffer();
    THEN("Loads are isolated.") {
      REQUIRE(prog.getNumConnectedNodes(21) == 0);
      REQUIRE(prog.getNumConnectedNodes(47) == 0);
      REQUIRE(prog.getNumConnectedNodes(73) == 0);
    }
    THEN("Loads' children are now store parents' children.") {
      REQUIRE(prog.edgeExists(10, 26) == 1);
      REQUIRE(prog.edgeExists(36, 52) == 1);
      REQUIRE(prog.edgeExists(62, 78) == 1);
    }
  }
}
