#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test removeInductionVariable w/ Triad", "[triad]")
{
  GIVEN("Test Triad w/ Input Size 128")
  {
    std::string bench("triad-128");
    std::string trace_file("inputs/triad-128-trace");
    std::string config_file("inputs/config-triad-p2-u2-P1");

    ScratchpadDatapath *acc;
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
SCENARIO("Test removeInductionVariable w/ Reduction", "[reduction]")
{
  GIVEN("Test Reduction w/ Input Size 128")
  {
    std::string bench("reduction-128");
    std::string trace_file("inputs/reduction-128-trace");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

    ScratchpadDatapath *acc;
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
SCENARIO("Test removeInductionVariable w/ Pp_scan", "[pp_scan]")
{
  GIVEN("Test Pp_scan w/ Input Size 128")
  {
    std::string bench("pp_scan-128");
    std::string trace_file("inputs/pp_scan-128-trace");
    std::string config_file("inputs/config-pp_scan-p4-u4-P1");

    ScratchpadDatapath *acc;
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
        REQUIRE(acc->getMicroop(28) == LLVM_IR_IndexAdd);
        REQUIRE(acc->getMicroop(1492) == LLVM_IR_IndexAdd);
        REQUIRE(acc->getMicroop(1750) == LLVM_IR_IndexAdd);
      }
    }
  }
}

