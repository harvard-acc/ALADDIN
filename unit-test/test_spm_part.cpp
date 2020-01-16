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
      THEN("The partition index of a memory node should be 0 or 1.") {
        REQUIRE(prog.getBaseAddressLabel(9).compare("a_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(11).compare("b_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(15).compare("c_acc") == 0);
        REQUIRE(prog.nodes.at(9)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(11)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(15)->get_partition_index() == 0);
        REQUIRE(prog.getBaseAddressLabel(21).compare("a_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(23).compare("b_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(27).compare("c_acc") == 0);
        REQUIRE(prog.nodes.at(21)->get_partition_index() == 1);
        REQUIRE(prog.nodes.at(23)->get_partition_index() == 1);
        REQUIRE(prog.nodes.at(27)->get_partition_index() == 1);
        REQUIRE(prog.getBaseAddressLabel(33).compare("a_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(35).compare("b_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(39).compare("c_acc") == 0);
        REQUIRE(prog.nodes.at(33)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(35)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(39)->get_partition_index() == 0);
        REQUIRE(prog.getBaseAddressLabel(1497).compare("a_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(1499).compare("b_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(1503).compare("c_acc") == 0);
        REQUIRE(prog.nodes.at(1497)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(1499)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(1503)->get_partition_index() == 0);
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
      THEN("The partition index of a memory node should be 0, 1, 2 or 3.") {
        REQUIRE(prog.getBaseAddressLabel(7).compare("in_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(15).compare("in_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(23).compare("in_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(31).compare("in_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(1023).compare("in_acc") == 0);
        REQUIRE(prog.nodes.at(7)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(15)->get_partition_index() == 1);
        REQUIRE(prog.nodes.at(23)->get_partition_index() == 2);
        REQUIRE(prog.nodes.at(31)->get_partition_index() == 3);
        REQUIRE(prog.nodes.at(1023)->get_partition_index() == 3);
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
      THEN("The partition index of a memory node should be 0, 1, 2 or 3.") {
        REQUIRE(prog.getBaseAddressLabel(9).compare("bucket_acc") == 0);
        REQUIRE(prog.nodes.at(9)->get_partition_index() == 0);
        REQUIRE(prog.getBaseAddressLabel(15).compare("bucket_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(17).compare("bucket_acc") == 0);
        REQUIRE(prog.nodes.at(15)->get_partition_index() == 1);
        REQUIRE(prog.nodes.at(17)->get_partition_index() == 1);
        REQUIRE(prog.getBaseAddressLabel(1462).compare("bucket_acc") == 0);
        REQUIRE(prog.nodes.at(1462)->get_partition_index() == 0);
        REQUIRE(prog.getBaseAddressLabel(1463).compare("bucket_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(1464).compare("sum") == 0);
        REQUIRE(prog.getBaseAddressLabel(1488).compare("bucket2_acc") == 0);
        REQUIRE(prog.nodes.at(1463)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(1464)->get_partition_index() == 0);
        REQUIRE(prog.nodes.at(1488)->get_partition_index() == 0);
        REQUIRE(prog.getBaseAddressLabel(2821).compare("bucket_acc") == 0);
        REQUIRE(prog.nodes.at(2821)->get_partition_index() == 3);
        REQUIRE(prog.getBaseAddressLabel(2822).compare("sum") == 0);
        REQUIRE(prog.nodes.at(2822)->get_partition_index() == 3);
      }
    }
  }
}
