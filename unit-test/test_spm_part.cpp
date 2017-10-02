#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test scratchpadPartition w/ Triad", "[triad]") {
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
    WHEN("Test scratchpadPartition()") {
      acc->scratchpadPartition();
      THEN("The baseAddress of memory operations should be '*-0' or '*-1'.") {
        REQUIRE(prog.getBaseAddressLabel(3).compare("a") == 0);
        REQUIRE(prog.getBaseAddressLabel(5).compare("b") == 0);
        REQUIRE(prog.getBaseAddressLabel(9).compare("c") == 0);
        REQUIRE(prog.nodes.at(3)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(5)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(9)->get_partition_index() == 0);
        REQUIRE(prog.getBaseAddressLabel(15).compare("a") == 0);
        REQUIRE(prog.getBaseAddressLabel(17).compare("b") == 0);
        REQUIRE(prog.getBaseAddressLabel(21).compare("c") == 0);
        REQUIRE(prog.nodes.at(15)->get_partition_index() == 1);
        REQUIRE(prog.nodes.at(17)->get_partition_index() == 1);
        REQUIRE(prog.nodes.at(21)->get_partition_index() == 1);
        REQUIRE(prog.getBaseAddressLabel(27).compare("a") == 0);
        REQUIRE(prog.getBaseAddressLabel(29).compare("b") == 0);
        REQUIRE(prog.getBaseAddressLabel(33).compare("c") == 0);
        REQUIRE(prog.nodes.at(27)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(29)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(33)->get_partition_index() == 0);
        REQUIRE(prog.getBaseAddressLabel(1491).compare("a") == 0);
        REQUIRE(prog.getBaseAddressLabel(1493).compare("b") == 0);
        REQUIRE(prog.getBaseAddressLabel(1497).compare("c") == 0);
        REQUIRE(prog.nodes.at(1491)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(1493)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(1497)->get_partition_index() == 0);
      }
    }
  }
}
SCENARIO("Test scratchpadPartition w/ Reduction", "[reduction]") {
  GIVEN("Test Reduction w/ Input Size 128") {
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
    WHEN("Test scratchpadPartition()") {
      acc->scratchpadPartition();
      THEN("The baseAddress of memory operations should be '*-0/1/2/3'.") {
        REQUIRE(prog.getBaseAddressLabel(4).compare("in") == 0);
        REQUIRE(prog.getBaseAddressLabel(12).compare("in") == 0);
        REQUIRE(prog.getBaseAddressLabel(20).compare("in") == 0);
        REQUIRE(prog.getBaseAddressLabel(28).compare("in") == 0);
        REQUIRE(prog.getBaseAddressLabel(1020).compare("in") == 0);
        REQUIRE(prog.nodes.at(4)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(12)->get_partition_index() == 1);
        REQUIRE(prog.nodes.at(20)->get_partition_index() == 2);
        REQUIRE(prog.nodes.at(28)->get_partition_index() == 3);
        REQUIRE(prog.nodes.at(1020)->get_partition_index() == 3);
      }
    }
  }
}
SCENARIO("Test scratchpadPartition w/ pp_scan", "[pp_scan]") {
  GIVEN("Test pp_scan w/ Input Size 128") {
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
    WHEN("Test scratchpadPartition()") {
      acc->scratchpadPartition();
      THEN("The baseAddress of memory operations should be '*-0/1/2/3'.") {
        REQUIRE(prog.getBaseAddressLabel(11).compare("bucket") == 0);
        REQUIRE(prog.nodes.at(11)->get_partition_index() == 0);
        REQUIRE(prog.getBaseAddressLabel(13).compare("bucket") == 0);
        REQUIRE(prog.getBaseAddressLabel(15).compare("bucket") == 0);
        REQUIRE(prog.nodes.at(13)->get_partition_index() == 1);
        REQUIRE(prog.nodes.at(15)->get_partition_index() == 1);
        REQUIRE(prog.getBaseAddressLabel(1463).compare("bucket") == 0);
        REQUIRE(prog.nodes.at(1463)->get_partition_index() == 2);
        REQUIRE(prog.getBaseAddressLabel(1465).compare("bucket") == 0);
        REQUIRE(prog.getBaseAddressLabel(1467).compare("bucket") == 0);
        REQUIRE(prog.getBaseAddressLabel(1488).compare("bucket") == 0);
        REQUIRE(prog.nodes.at(1465)->get_partition_index() == 3);
        REQUIRE(prog.nodes.at(1467)->get_partition_index() == 3);
        REQUIRE(prog.nodes.at(1488)->get_partition_index() == 3);
        REQUIRE(prog.getBaseAddressLabel(1491).compare("sum") == 0);
        REQUIRE(prog.nodes.at(1491)->get_partition_index() == 1);
        REQUIRE(prog.getBaseAddressLabel(3169).compare("bucket") == 0);
        REQUIRE(prog.nodes.at(3169)->get_partition_index() == 3);
        REQUIRE(prog.getBaseAddressLabel(3167).compare("sum") == 0);
        REQUIRE(prog.nodes.at(3167)->get_partition_index() == 3);
      }
    }
  }
}
