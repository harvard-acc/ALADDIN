#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test DDDG Generation", "[dddg]")
{
  ScratchpadDatapath *acc;
  GIVEN("Test Triad w/ Input Size 128")
  {
    std::string bench("triad-128");
    std::string trace_file("inputs/triad-128-trace");
    std::string config_file("inputs/config-p2-u2-P1");

    Scratchpad *spad;
    WHEN("DDDG is generated.")
    {
      acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
      THEN("The Graph Size should match expectations.")
      {
        REQUIRE(acc->getNumOfNodes() == 1537);
        REQUIRE(acc->getNumOfEdges() == 1791);
      }
    }
  }
}

SCENARIO("Test removeInductionVariable", "[rmInd]")
{
  ScratchpadDatapath *acc;
  GIVEN("Test Triad w/ Input Size 128")
  {
    std::string bench("triad-128");
    std::string trace_file("inputs/triad-128-trace");
    std::string config_file("inputs/config-p2-u2-P1");

    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
    spad = new Scratchpad(1);
    acc->setScratchpad(spad);
    acc->setGlobalGraph();
    WHEN("Test removeInductionDependence()")
    {
      acc->removeInductionDependence();
      THEN("Addition w/ Induction Variable should be converted to LLVM_IR_IndexAdd.")
      {
        REQUIRE(acc->getMicroop(10) == LLVM_IR_IndexAdd);
        REQUIRE(acc->getMicroop(22) == LLVM_IR_IndexAdd);
        REQUIRE(acc->getMicroop(1534) == LLVM_IR_IndexAdd);
      }
    }
  }
}

SCENARIO("Test removePhiNodes", "[rmPhi]")
{
  ScratchpadDatapath *acc;
  GIVEN("Test Triad w/ Input Size 128")
  {
    std::string bench("triad-128");
    std::string trace_file("inputs/triad-128-trace");
    std::string config_file("inputs/config-p2-u2-P1");

    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
    spad = new Scratchpad(1);
    acc->setScratchpad(spad);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    WHEN("Test removePhiNodes()")
    {
      acc->removePhiNodes();
      THEN("Phi Nodes in the DDDG should be isolated.")
      {
        REQUIRE(acc->getNumOfConnectedNodes(13) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(25) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1525) == 0);
      }
    }
  }
}

SCENARIO("Test initBaseAddress", "[initBase]")
{
  ScratchpadDatapath *acc;
  GIVEN("Test Triad w/ Input Size 128")
  {
    std::string bench("triad-128");
    std::string trace_file("inputs/triad-128-trace");
    std::string config_file("inputs/config-p2-u2-P1");

    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
    REQUIRE(acc->getNumOfNodes() == 1537);
    REQUIRE(acc->getNumOfEdges() == 1791);

    spad = new Scratchpad(1);
    acc->setScratchpad(spad);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    WHEN("Test initBaseAddress()")
    {
      acc->initBaseAddress();
      THEN("The baseAddress of memory operations should be either 'a', 'b' or 'c' for Triad.")
      {
        REQUIRE(acc->getBaseAddressLabel(3).compare("a") == 0);
        REQUIRE(acc->getBaseAddressLabel(5).compare("b") == 0);
        REQUIRE(acc->getBaseAddressLabel(9).compare("c") == 0);
        REQUIRE(acc->getBaseAddressLabel(1491).compare("a") == 0);
        REQUIRE(acc->getBaseAddressLabel(1493).compare("b") == 0);
        REQUIRE(acc->getBaseAddressLabel(1497).compare("c") == 0);
      }
    }
  }
}

SCENARIO("Test scratchpadPartition", "[spadPart]")
{
  ScratchpadDatapath *acc;
  GIVEN("Test Triad w/ Input Size 128, cyclic partition with a factor of 2, loop unrolling with a factor of 2, enable loop pipelining")
  {
    std::string bench("triad-128");
    std::string trace_file("inputs/triad-128-trace");
    std::string config_file("inputs/config-p2-u2-P1");

    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
    spad = new Scratchpad(1);
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
