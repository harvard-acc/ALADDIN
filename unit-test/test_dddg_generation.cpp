#include "unit-test-config.h"

SCENARIO("Test DDDG Generation w/ Triad", "[triad]") {
  GIVEN("Test Triad w/ Input Size 128") {
    std::string bench("outputs/triad-128");
    std::string trace_file("inputs/triad-128-trace.gz");
    std::string config_file("inputs/config-triad-p2-u2-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    WHEN("DDDG is generated.") {
      acc = new ScratchpadDatapath(bench, trace_file, config_file);
      acc->buildDddg();
      THEN("The Graph Size should match expectations.") {
        REQUIRE(acc->getNumOfNodes() == 1537);
        REQUIRE(acc->getNumOfEdges() == 1791);
      }
    }
  }
}
SCENARIO("Test DDDG Generation w/ Reduction", "[reduction]") {
  GIVEN("Test Reduction w/ Input Size 128") {
    std::string bench("outputs/reduction-128");
    std::string trace_file("inputs/reduction-128-trace.gz");
    std::string config_file("inputs/config-reduction-p4-u4-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    WHEN("DDDG is generated.") {
      acc = new ScratchpadDatapath(bench, trace_file, config_file);
      acc->buildDddg();
      THEN("The Graph Size should match expectations.") {
        REQUIRE(acc->getNumOfNodes() == 1026);
        REQUIRE(acc->getNumOfEdges() == 1151);
      }
    }
  }
}
SCENARIO("Test DDDG Generation w/ Pp_scan", "[pp_scan]") {
  GIVEN("Test Pp_scan w/ Input Size 128") {
    std::string bench("outputs/pp_scan-128");
    std::string trace_file("inputs/pp_scan-128-trace.gz");
    std::string config_file("inputs/config-pp_scan-p4-u4-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    WHEN("DDDG is generated.") {
      acc = new ScratchpadDatapath(bench, trace_file, config_file);
      acc->buildDddg();
      THEN("The Graph Size should match expectations.") {
        REQUIRE(acc->getNumOfNodes() == 3176);
        REQUIRE(acc->getNumOfEdges() == 4630);
      }
    }
  }
}
SCENARIO("Test DDDG Generation w/ aes", "[aes]") {
  GIVEN("Test MachSuite aes-aes") {
    std::string bench("outputs/aes-aes");
    std::string trace_file("inputs/aes-aes-trace.gz");
    std::string config_file("inputs/config-aes-aes");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    WHEN("DDDG is generated.") {
      acc = new ScratchpadDatapath(bench, trace_file, config_file);
      acc->buildDddg();
      THEN("The Graph Size should match expectations.") {
        REQUIRE(acc->getNumOfNodes() == 14820);
        REQUIRE(acc->getNumOfEdges() == 21375);
      }
    }
  }
}
