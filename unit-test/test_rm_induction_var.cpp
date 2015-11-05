#include "unit-test-config.h"

SCENARIO("Test removeInductionVariable w/ Triad", "[triad]") {
  GIVEN("Test Triad w/ Input Size 128") {
    std::string bench("outputs/triad-128");
    std::string trace_file("inputs/triad-128-trace.gz");
    std::string config_file("inputs/config-triad-p2-u2-P1");

    trace_file = root_dir + trace_file;
    config_file = root_dir + config_file;

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    WHEN("Test removeInductionDependence()") {
      acc->removeInductionDependence();
      THEN("Addition w/ Induction Variable should be converted to "
           "LLVM_IR_IndexAdd.") {
        REQUIRE(acc->getMicroop(10) == LLVM_IR_IndexAdd);
        REQUIRE(acc->getMicroop(22) == LLVM_IR_IndexAdd);
        REQUIRE(acc->getMicroop(1534) == LLVM_IR_IndexAdd);
      }
    }
  }
}
SCENARIO("Test removeInductionVariable w/ Reduction", "[reduction]") {
  GIVEN("Test Reduction w/ Input Size 128") {
    std::string bench("outputs/reduction-128");
    std::string trace_file("inputs/reduction-128-trace.gz");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

    trace_file = root_dir + trace_file;
    config_file = root_dir + config_file;

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    WHEN("Test removeInductionDependence()") {
      acc->removeInductionDependence();
      THEN("Addition w/ Induction Variable should be converted to "
           "LLVM_IR_IndexAdd.") {
        REQUIRE(acc->getMicroop(6) == LLVM_IR_IndexAdd);
        REQUIRE(acc->getMicroop(22) == LLVM_IR_IndexAdd);
        REQUIRE(acc->getMicroop(1022) == LLVM_IR_IndexAdd);
      }
    }
  }
}
SCENARIO("Test removeInductionVariable w/ Pp_scan", "[pp_scan]") {
  GIVEN("Test Pp_scan w/ Input Size 128") {
    std::string bench("outputs/pp_scan-128");
    std::string trace_file("inputs/pp_scan-128-trace.gz");
    std::string config_file("inputs/config-pp_scan-p4-u4-P1");

    trace_file = root_dir + trace_file;
    config_file = root_dir + config_file;

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    WHEN("Test removeInductionDependence()") {
      acc->removeInductionDependence();
      THEN("Addition w/ Induction Variable should be converted to "
           "LLVM_IR_IndexAdd.") {
        REQUIRE(acc->getMicroop(28) == LLVM_IR_IndexAdd);
        REQUIRE(acc->getMicroop(1492) == LLVM_IR_IndexAdd);
        REQUIRE(acc->getMicroop(1750) == LLVM_IR_IndexAdd);
      }
    }
  }
}
