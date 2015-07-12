#include "unit-test-config.h"

SCENARIO("Test initBaseAddress w/ c[i]=a[i] case", "[equal]") {
  GIVEN("Test c[i]=a[i] case w/ Input Size 32") {
    std::string bench("outputs/triad-initbase");
    std::string trace_file("inputs/triad-initbase-trace.gz");
    std::string config_file("inputs/config-triad-initbase-p1-u1-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    WHEN("Test initBaseAddress()") {
      acc->initBaseAddress();
      THEN("The baseAddress of memory operations should be either "
           "'a', 'b' or 'c' for Triad.") {
        REQUIRE(acc->getBaseAddressLabel(3).compare("a") == 0);
        REQUIRE(acc->getBaseAddressLabel(5).compare("c") == 0);
        REQUIRE(acc->getBaseAddressLabel(11).compare("a") == 0);
        REQUIRE(acc->getBaseAddressLabel(13).compare("c") == 0);
        REQUIRE(acc->getBaseAddressLabel(19).compare("a") == 0);
        REQUIRE(acc->getBaseAddressLabel(21).compare("c") == 0);
      }
    }
  }
}

SCENARIO("Test initBaseAddress w/ Triad", "[triad]") {
  GIVEN("Test Triad w/ Input Size 128") {
    std::string bench("outputs/triad-128");
    std::string trace_file("inputs/triad-128-trace.gz");
    std::string config_file("inputs/config-triad-p2-u2-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    WHEN("Test initBaseAddress()") {
      acc->initBaseAddress();
      THEN("The baseAddress of memory operations should be either "
           "'a', 'b' or 'c' for Triad.") {
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
SCENARIO("Test initBaseAddress w/ Reduction", "[reduction]") {
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
    WHEN("Test initBaseAddress()") {
      acc->initBaseAddress();
      THEN("The baseAddress of memory operations should be 'in' for "
           "Reduction.") {
        REQUIRE(acc->getBaseAddressLabel(4).compare("in") == 0);
        REQUIRE(acc->getBaseAddressLabel(12).compare("in") == 0);
        REQUIRE(acc->getBaseAddressLabel(1020).compare("in") == 0);
      }
    }
  }
}
SCENARIO("Test initBaseAddress w/ Pp_scan", "[pp_scan]") {
  GIVEN("Test Pp_scan w/ Input Size 128") {
    std::string bench("outputs/pp_scan-128");
    std::string trace_file("inputs/pp_scan-128-trace.gz");
    std::string config_file("inputs/config-pp_scan-p4-u4-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    WHEN("Test initBaseAddress()") {
      acc->initBaseAddress();
      THEN("The baseAddress of memory operations should be 'bucket' or 'sum' "
           "for pp_scan.") {
        REQUIRE(acc->getBaseAddressLabel(11).compare("bucket") == 0);
        REQUIRE(acc->getBaseAddressLabel(13).compare("bucket") == 0);
        REQUIRE(acc->getBaseAddressLabel(15).compare("bucket") == 0);
        REQUIRE(acc->getBaseAddressLabel(1463).compare("bucket") == 0);
        REQUIRE(acc->getBaseAddressLabel(1465).compare("bucket") == 0);
        REQUIRE(acc->getBaseAddressLabel(1467).compare("bucket") == 0);
        REQUIRE(acc->getBaseAddressLabel(1488).compare("bucket") == 0);
        REQUIRE(acc->getBaseAddressLabel(1491).compare("sum") == 0);
        REQUIRE(acc->getBaseAddressLabel(3169).compare("bucket") == 0);
        REQUIRE(acc->getBaseAddressLabel(3167).compare("sum") == 0);
      }
    }
  }
}
