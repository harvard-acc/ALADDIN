#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test removeInductionVariable w/ Triad", "[triad]") {
  GIVEN("Test Triad w/ Input Size 128") {
    std::string bench("outputs/triad-128");
    std::string trace_file("inputs/triad-128-trace.gz");
    std::string config_file("inputs/config-triad-p2-u2-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    auto& prog = acc->getProgram();
    acc->buildDddg();
    WHEN("Test removeInductionDependence()") {
      acc->removeInductionDependence();
      THEN("Addition w/ Induction Variable should be converted to "
           "LLVM_IR_IndexAdd.") {
        REQUIRE(prog.nodes.at(10)->get_microop() == LLVM_IR_IndexAdd);
        REQUIRE(prog.nodes.at(22)->get_microop() == LLVM_IR_IndexAdd);
        REQUIRE(prog.nodes.at(1534)->get_microop() == LLVM_IR_IndexAdd);
      }
    }
  }
}
SCENARIO("Test removeInductionVariable w/ Reduction", "[reduction]") {
  GIVEN("Test Reduction w/ Input Size 128") {
    std::string bench("outputs/reduction-128");
    std::string trace_file("inputs/reduction-128-trace.gz");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    auto& prog = acc->getProgram();
    acc->buildDddg();
    WHEN("Test removeInductionDependence()") {
      acc->removeInductionDependence();
      THEN("Addition w/ Induction Variable should be converted to "
           "LLVM_IR_IndexAdd.") {
        REQUIRE(prog.nodes.at(6)->get_microop() == LLVM_IR_IndexAdd);
        REQUIRE(prog.nodes.at(22)->get_microop() == LLVM_IR_IndexAdd);
        REQUIRE(prog.nodes.at(1022)->get_microop() == LLVM_IR_IndexAdd);
      }
    }
  }
}
SCENARIO("Test removeInductionVariable w/ Pp_scan", "[pp_scan]") {
  GIVEN("Test Pp_scan w/ Input Size 128") {
    std::string bench("outputs/pp_scan-128");
    std::string trace_file("inputs/pp_scan-128-trace.gz");
    std::string config_file("inputs/config-pp_scan-p4-u4-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    auto& prog = acc->getProgram();
    acc->buildDddg();
    WHEN("Test removeInductionDependence()") {
      acc->removeInductionDependence();
      THEN("Addition w/ Induction Variable should be converted to "
           "LLVM_IR_IndexAdd.") {
        REQUIRE(prog.nodes.at(28)->get_microop() == LLVM_IR_IndexAdd);
        REQUIRE(prog.nodes.at(1492)->get_microop() == LLVM_IR_IndexAdd);
        REQUIRE(prog.nodes.at(1750)->get_microop() == LLVM_IR_IndexAdd);
      }
    }
  }
}
