#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test scratchpadPartition w/ Triad", "[triad]")
{
  GIVEN("Test Triad w/ Input Size 128, cyclic partition with a factor of 2, "
        "loop unrolling with a factor of 2, enable loop pipelining")
  {
    std::string bench("outputs/triad-128");
    std::string trace_file("inputs/triad-128-trace");
    std::string config_file("inputs/config-triad-p2-u2-P1");

    ScratchpadDatapath *acc;
    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
    spad = new Scratchpad(1, CYCLE_TIME);
    acc->setScratchpad(spad);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    WHEN("Test initBaseAddress()")
    {
      acc->scratchpadPartition();
      THEN("The baseAddress of memory operations should be '*-0' or '*-1'.")
      {
        REQUIRE(acc->getBaseAddressLabel(3).compare("a-0") == 0);
        REQUIRE(acc->getBaseAddressLabel(5).compare("b-0") == 0);
        REQUIRE(acc->getBaseAddressLabel(9).compare("c-0") == 0);
        REQUIRE(acc->getBaseAddressLabel(15).compare("a-1") == 0);
        REQUIRE(acc->getBaseAddressLabel(17).compare("b-1") == 0);
        REQUIRE(acc->getBaseAddressLabel(21).compare("c-1") == 0);
        REQUIRE(acc->getBaseAddressLabel(27).compare("a-0") == 0);
        REQUIRE(acc->getBaseAddressLabel(29).compare("b-0") == 0);
        REQUIRE(acc->getBaseAddressLabel(33).compare("c-0") == 0);
        REQUIRE(acc->getBaseAddressLabel(1491).compare("a-0") == 0);
        REQUIRE(acc->getBaseAddressLabel(1493).compare("b-0") == 0);
        REQUIRE(acc->getBaseAddressLabel(1497).compare("c-0") == 0);
      }
    }
  }
}
SCENARIO("Test scratchpadPartition w/ Reduction", "[reduction]")
{
  GIVEN("Test Reduction w/ Input Size 128")
  {
    std::string bench("outputs/reduction-128");
    std::string trace_file("inputs/reduction-128-trace");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

    ScratchpadDatapath *acc;
    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
    spad = new Scratchpad(1, CYCLE_TIME);
    acc->setScratchpad(spad);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    WHEN("Test initBaseAddress()")
    {
      acc->scratchpadPartition();
      THEN("The baseAddress of memory operations should be '*-0/1/2/3'.")
      {
        REQUIRE(acc->getBaseAddressLabel(4).compare("in-0") == 0);
        REQUIRE(acc->getBaseAddressLabel(12).compare("in-1") == 0);
        REQUIRE(acc->getBaseAddressLabel(20).compare("in-2") == 0);
        REQUIRE(acc->getBaseAddressLabel(28).compare("in-3") == 0);
        REQUIRE(acc->getBaseAddressLabel(1020).compare("in-3") == 0);
      }
    }
  }
}
SCENARIO("Test scratchpadPartition w/ pp_scan", "[pp_scan]")
{
  GIVEN("Test pp_scan w/ Input Size 128")
  {
    std::string bench("outputs/pp_scan-128");
    std::string trace_file("inputs/pp_scan-128-trace");
    std::string config_file("inputs/config-pp_scan-p4-u4-P1");

    ScratchpadDatapath *acc;
    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
    spad = new Scratchpad(1, CYCLE_TIME);
    acc->setScratchpad(spad);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    WHEN("Test initBaseAddress()")
    {
      acc->scratchpadPartition();
      THEN("The baseAddress of memory operations should be '*-0/1/2/3'.")
      {
        REQUIRE(acc->getBaseAddressLabel(11).compare("bucket-0") == 0);
        REQUIRE(acc->getBaseAddressLabel(13).compare("bucket-1") == 0);
        REQUIRE(acc->getBaseAddressLabel(15).compare("bucket-1") == 0);
        REQUIRE(acc->getBaseAddressLabel(1463).compare("bucket-2") == 0);
        REQUIRE(acc->getBaseAddressLabel(1465).compare("bucket-3") == 0);
        REQUIRE(acc->getBaseAddressLabel(1467).compare("bucket-3") == 0);
        REQUIRE(acc->getBaseAddressLabel(1488).compare("bucket-3") == 0);
        REQUIRE(acc->getBaseAddressLabel(1491).compare("sum-1") == 0);
        REQUIRE(acc->getBaseAddressLabel(3169).compare("bucket-3") == 0);
        REQUIRE(acc->getBaseAddressLabel(3167).compare("sum-3") == 0);
      }
    }
  }
}

