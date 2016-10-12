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
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    WHEN("Test scratchpadPartition()") {
      acc->scratchpadPartition();
      THEN("The baseAddress of memory operations should be '*-0' or '*-1'.") {
        REQUIRE(acc->getBaseAddressLabel(3).compare("a") == 0);
        REQUIRE(acc->getBaseAddressLabel(5).compare("b") == 0);
        REQUIRE(acc->getBaseAddressLabel(9).compare("c") == 0);
        REQUIRE(acc->getPartitionIndex(3) == 0);
        REQUIRE(acc->getPartitionIndex(5) == 0);
        REQUIRE(acc->getPartitionIndex(9) == 0);
        REQUIRE(acc->getBaseAddressLabel(15).compare("a") == 0);
        REQUIRE(acc->getBaseAddressLabel(17).compare("b") == 0);
        REQUIRE(acc->getBaseAddressLabel(21).compare("c") == 0);
        REQUIRE(acc->getPartitionIndex(15) == 1);
        REQUIRE(acc->getPartitionIndex(17) == 1);
        REQUIRE(acc->getPartitionIndex(21) == 1);
        REQUIRE(acc->getBaseAddressLabel(27).compare("a") == 0);
        REQUIRE(acc->getBaseAddressLabel(29).compare("b") == 0);
        REQUIRE(acc->getBaseAddressLabel(33).compare("c") == 0);
        REQUIRE(acc->getPartitionIndex(27) == 0);
        REQUIRE(acc->getPartitionIndex(29) == 0);
        REQUIRE(acc->getPartitionIndex(33) == 0);
        REQUIRE(acc->getBaseAddressLabel(1491).compare("a") == 0);
        REQUIRE(acc->getBaseAddressLabel(1493).compare("b") == 0);
        REQUIRE(acc->getBaseAddressLabel(1497).compare("c") == 0);
        REQUIRE(acc->getPartitionIndex(1491) == 0);
        REQUIRE(acc->getPartitionIndex(1493) == 0);
        REQUIRE(acc->getPartitionIndex(1497) == 0);
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
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    WHEN("Test scratchpadPartition()") {
      acc->scratchpadPartition();
      THEN("The baseAddress of memory operations should be '*-0/1/2/3'.") {
        REQUIRE(acc->getBaseAddressLabel(4).compare("in") == 0);
        REQUIRE(acc->getBaseAddressLabel(12).compare("in") == 0);
        REQUIRE(acc->getBaseAddressLabel(20).compare("in") == 0);
        REQUIRE(acc->getBaseAddressLabel(28).compare("in") == 0);
        REQUIRE(acc->getBaseAddressLabel(1020).compare("in") == 0);
        REQUIRE(acc->getPartitionIndex(4) == 0);
        REQUIRE(acc->getPartitionIndex(12) == 1);
        REQUIRE(acc->getPartitionIndex(20) == 2);
        REQUIRE(acc->getPartitionIndex(28) == 3);
        REQUIRE(acc->getPartitionIndex(1020) == 3);
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
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    WHEN("Test scratchpadPartition()") {
      acc->scratchpadPartition();
      THEN("The baseAddress of memory operations should be '*-0/1/2/3'.") {
        REQUIRE(acc->getBaseAddressLabel(11).compare("bucket") == 0);
        REQUIRE(acc->getPartitionIndex(11) == 0);
        REQUIRE(acc->getBaseAddressLabel(13).compare("bucket") == 0);
        REQUIRE(acc->getBaseAddressLabel(15).compare("bucket") == 0);
        REQUIRE(acc->getPartitionIndex(13) == 1);
        REQUIRE(acc->getPartitionIndex(15) == 1);
        REQUIRE(acc->getBaseAddressLabel(1463).compare("bucket") == 0);
        REQUIRE(acc->getPartitionIndex(1463) == 2);
        REQUIRE(acc->getBaseAddressLabel(1465).compare("bucket") == 0);
        REQUIRE(acc->getBaseAddressLabel(1467).compare("bucket") == 0);
        REQUIRE(acc->getBaseAddressLabel(1488).compare("bucket") == 0);
        REQUIRE(acc->getPartitionIndex(1465) == 3);
        REQUIRE(acc->getPartitionIndex(1467) == 3);
        REQUIRE(acc->getPartitionIndex(1488) == 3);
        REQUIRE(acc->getBaseAddressLabel(1491).compare("sum") == 0);
        REQUIRE(acc->getPartitionIndex(1491) == 1);
        REQUIRE(acc->getBaseAddressLabel(3169).compare("bucket") == 0);
        REQUIRE(acc->getPartitionIndex(3169) == 3);
        REQUIRE(acc->getBaseAddressLabel(3167).compare("sum") == 0);
        REQUIRE(acc->getPartitionIndex(3167) == 3);
      }
    }
  }
}
