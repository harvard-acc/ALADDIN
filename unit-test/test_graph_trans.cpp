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
    std::string config_file("inputs/config-triad-p2-u2-P1");

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
  GIVEN("Test Reduction w/ Input Size 128")
  {
    std::string bench("reduction-128");
    std::string trace_file("inputs/reduction-128-trace");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

    Scratchpad *spad;
    WHEN("DDDG is generated.")
    {
      acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
      THEN("The Graph Size should match expectations.")
      {
        REQUIRE(acc->getNumOfNodes() == 1026);
        REQUIRE(acc->getNumOfEdges() == 1151);
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
    std::string config_file("inputs/config-triad-p2-u2-P1");

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
  GIVEN("Test Reduction w/ Input Size 128")
  {
    std::string bench("reduction-128");
    std::string trace_file("inputs/reduction-128-trace");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

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
        REQUIRE(acc->getMicroop(6) == LLVM_IR_IndexAdd);
        REQUIRE(acc->getMicroop(22) == LLVM_IR_IndexAdd);
        REQUIRE(acc->getMicroop(1022) == LLVM_IR_IndexAdd);
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
    std::string config_file("inputs/config-triad-p2-u2-P1");

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
  GIVEN("Test Reduction w/ Input Size 128")
  {
    std::string bench("reduction-128");
    std::string trace_file("inputs/reduction-128-trace");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

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
        REQUIRE(acc->getNumOfConnectedNodes(1) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(10) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1009) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1018) == 0);
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
    std::string config_file("inputs/config-triad-p2-u2-P1");

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
  GIVEN("Test Reduction w/ Input Size 128")
  {
    std::string bench("reduction-128");
    std::string trace_file("inputs/reduction-128-trace");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
    spad = new Scratchpad(1);
    acc->setScratchpad(spad);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    WHEN("Test initBaseAddress()")
    {
      acc->initBaseAddress();
      THEN("The baseAddress of memory operations should be 'in' for Reduction.")
      {
        REQUIRE(acc->getBaseAddressLabel(4).compare("in") == 0);
        REQUIRE(acc->getBaseAddressLabel(12).compare("in") == 0);
        REQUIRE(acc->getBaseAddressLabel(1020).compare("in") == 0);
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
    std::string config_file("inputs/config-triad-p2-u2-P1");

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
  GIVEN("Test Reduction w/ Input Size 128")
  {
    std::string bench("reduction-128");
    std::string trace_file("inputs/reduction-128-trace");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

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

SCENARIO("Test loopUnrolling", "[loopUnrolling]")
{
  ScratchpadDatapath *acc;
  GIVEN("Test Triad w/ Input Size 128, cyclic partition with a factor of 2, loop unrolling with a factor of 2, enable loop pipelining")
  {
    std::string bench("triad-128");
    std::string trace_file("inputs/triad-128-trace");
    std::string config_file("inputs/config-triad-p2-u2-P1");

    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
    spad = new Scratchpad(1);
    acc->setScratchpad(spad);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    WHEN("Test loopUnrolling()")
    {
      acc->loopUnrolling();
      THEN("Unrolled loop boundary should match the expectations.")
      {
        REQUIRE(acc->getUnrolledLoopBoundary(1) == 24);
        REQUIRE(acc->getUnrolledLoopBoundary(2) == 48);
        REQUIRE(acc->getUnrolledLoopBoundary(3) == 72);
        REQUIRE(acc->getUnrolledLoopBoundary(21) == 504);
        REQUIRE(acc->getUnrolledLoopBoundary(64) == 1536);
      }
      THEN("Branch nodes inside unrolled iterations are isolated.")
      {
        REQUIRE(acc->getNumOfConnectedNodes(12) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(24) != 0);
        REQUIRE(acc->getNumOfConnectedNodes(36) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1524) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(1536) != 0);
      }
      THEN("Branch edges are added between boundary branch nodes and nodes in the next unrolled iterations.")
      {
        REQUIRE(acc->doesEdgeExist(2, 9) == 1);
        REQUIRE(acc->doesEdgeExist(24, 27) == 1);
        REQUIRE(acc->doesEdgeExist(48, 53) == 1);
        REQUIRE(acc->doesEdgeExist(72, 81) == 1);
        REQUIRE(acc->doesEdgeExist(504, 513) == 1);
        REQUIRE(acc->doesEdgeExist(1512, 1527) == 1);
      }
      THEN("Branch edges are added between boundary nodes and nodes inside the current unrolled iterations.")
      {
        REQUIRE(acc->doesEdgeExist(3, 24) == 1);
        REQUIRE(acc->doesEdgeExist(27, 48) == 1);
        REQUIRE(acc->doesEdgeExist(53, 72) == 1);
        REQUIRE(acc->doesEdgeExist(501, 504) == 1);
        REQUIRE(acc->doesEdgeExist(1503, 1512) == 1);
      }
    }
  }
  GIVEN("Test Reduction w/ Input Size 128")
  {
    std::string bench("reduction-128");
    std::string trace_file("inputs/reduction-128-trace");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
    spad = new Scratchpad(1);
    acc->setScratchpad(spad);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    WHEN("Test loopUnrolling()")
    {
      acc->loopUnrolling();
      THEN("Unrolled loop boundary should match the expectations.")
      {
        REQUIRE(acc->getUnrolledLoopBoundary(1) == 32);
        REQUIRE(acc->getUnrolledLoopBoundary(2) == 64);
        REQUIRE(acc->getUnrolledLoopBoundary(32) == 1024);
      }
      THEN("Branch nodes inside unrolled iterations are isolated.")
      {
        REQUIRE(acc->getNumOfConnectedNodes(8) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(24) == 0);
        REQUIRE(acc->getNumOfConnectedNodes(32) != 0);
        REQUIRE(acc->getNumOfConnectedNodes(1016) == 0);
      }
      THEN("Branch edges are added between boundary branch nodes and nodes in the next unrolled iterations.")
      {
        REQUIRE(acc->doesEdgeExist(3, 5) == 1);
        REQUIRE(acc->doesEdgeExist(32, 37) == 1);
        REQUIRE(acc->doesEdgeExist(32, 45) == 1);
        REQUIRE(acc->doesEdgeExist(992, 1021) == 1);
      }
      THEN("Branch edges are added between boundary nodes and nodes inside the current unrolled iterations.")
      {
        REQUIRE(acc->doesEdgeExist(5, 32) == 1);
        REQUIRE(acc->doesEdgeExist(29, 32) == 1);
        REQUIRE(acc->doesEdgeExist(1021, 1024) == 1);
      }
    }
  }
}
SCENARIO("Test treeHeightReduction", "[tree]")
{
  ScratchpadDatapath *acc;
  GIVEN("Test Reduction w/ Input Size 128")
  {
    std::string bench("reduction-128");
    std::string trace_file("inputs/reduction-128-trace");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
    spad = new Scratchpad(1);
    acc->setScratchpad(spad);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    acc->loopUnrolling();
    WHEN("before loopUnrolling()")
    {
      THEN("Distance between the last sums in two unrolled iterations should be 4.")
      {
        REQUIRE(acc->shortestDistanceBetweenNodes(29,61) == 4);
        REQUIRE(acc->shortestDistanceBetweenNodes(61,93) == 4);
        REQUIRE(acc->shortestDistanceBetweenNodes(989,1021) == 4);
      }
    }
    WHEN("Test loopUnrolling()")
    {
      acc->treeHeightReduction();
      THEN("Distance between the last sums in two unrolled iterations should be 1.")
      {
        REQUIRE(acc->shortestDistanceBetweenNodes(29,61) == 1);
        REQUIRE(acc->shortestDistanceBetweenNodes(61,93) == 1);
        REQUIRE(acc->shortestDistanceBetweenNodes(989,1021) == 1);
      }
    }
  }
}
SCENARIO("Test loopPipelining", "[loopPipelining]")
{
  ScratchpadDatapath *acc;
  GIVEN("Test Triad w/ Input Size 128, cyclic partition with a factor of 2, loop unrolling with a factor of 2, enable loop pipelining")
  {
    std::string bench("triad-128");
    std::string trace_file("inputs/triad-128-trace");
    std::string config_file("inputs/config-triad-p2-u2-P1");

    Scratchpad *spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file, CYCLE_TIME);
    spad = new Scratchpad(1);
    acc->setScratchpad(spad);
    acc->setGlobalGraph();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    acc->initBaseAddress();
    acc->scratchpadPartition();
    acc->loopUnrolling();
    WHEN("Test loopPipelining()")
    {
      acc->loopPipelining();
      THEN("Dependences exist between first instructions of pipelined iterations.")
      {
        REQUIRE(acc->doesEdgeExist(2, 26) == 1);
        REQUIRE(acc->doesEdgeExist(26, 50) == 1);
        REQUIRE(acc->doesEdgeExist(50, 74) == 1);
        REQUIRE(acc->doesEdgeExist(482, 506) == 1);
        REQUIRE(acc->doesEdgeExist(1490, 1514) == 1);
      }
      THEN("Dependences exist between the first instructions and the rest of instructions in the same loop region.")
      {
        REQUIRE(acc->doesEdgeExist(2, 9) == 1);
        REQUIRE(acc->doesEdgeExist(26, 29) == 1);
        REQUIRE(acc->doesEdgeExist(26, 33) == 1);
        REQUIRE(acc->doesEdgeExist(50, 54) == 1);
        REQUIRE(acc->doesEdgeExist(482, 487) == 1);
        REQUIRE(acc->doesEdgeExist(1490, 1505) == 1);
      }
      THEN("Branch edges are removed between boundary branch nodes and nodes in the next unrolled iterations.")
      {
        REQUIRE(acc->doesEdgeExist(24, 27) == 0);
        REQUIRE(acc->doesEdgeExist(48, 53) == 0);
        REQUIRE(acc->doesEdgeExist(72, 81) == 0);
        REQUIRE(acc->doesEdgeExist(504, 513) == 0);
        REQUIRE(acc->doesEdgeExist(1512, 1527) == 0);
      }
    }
  }
}
