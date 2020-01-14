#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test initBaseAddress w/ c[i]=a[i] case", "[equal]") {
  GIVEN("Test c[i]=a[i] case w/ Input Size 32") {
    std::string bench("outputs/triad-initbase");
    std::string trace_file("inputs/triad-initbase-trace.gz");
    std::string config_file("inputs/config-triad-initbase-p1-u1-P1");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    auto& prog = acc->getProgram();
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    WHEN("Test initBaseAddress()") {
      acc->initBaseAddress();
      THEN("The baseAddress of memory operations should be either "
           "'a_acc', 'b_acc' or 'c_acc' for Triad.") {
        REQUIRE(prog.getBaseAddressLabel(9).compare("a_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(15).compare("c_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(11).compare("b_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(27).compare("c_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(21).compare("a_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(39).compare("c_acc") == 0);
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
    auto& prog = acc->getProgram();
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    WHEN("Test initBaseAddress()") {
      acc->initBaseAddress();
      THEN("The baseAddress of memory operations should be either "
           "'a_acc', 'b_acc' or 'c_acc' for Triad.") {
        REQUIRE(prog.getBaseAddressLabel(9).compare("a_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(11).compare("b_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(15).compare("c_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(1533).compare("a_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(1535).compare("b_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(1539).compare("c_acc") == 0);
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
    auto& prog = acc->getProgram();
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    WHEN("Test initBaseAddress()") {
      acc->initBaseAddress();
      THEN("The baseAddress of memory operations should be 'in' for "
           "Reduction.") {
        REQUIRE(prog.getBaseAddressLabel(7).compare("in_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(15).compare("in_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(1023).compare("in_acc") == 0);
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
    auto& prog = acc->getProgram();
    acc->buildDddg();
    acc->removeInductionDependence();
    acc->removePhiNodes();
    WHEN("Test initBaseAddress()") {
      acc->initBaseAddress();
      THEN("The baseAddress of memory operations should be 'bucket' or 'sum' "
           "for pp_scan.") {
        REQUIRE(prog.getBaseAddressLabel(9).compare("bucket_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(15).compare("bucket_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(25).compare("bucket_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(1463).compare("bucket_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(1474).compare("bucket_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(1485).compare("bucket_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(1496).compare("bucket_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(1559).compare("sum") == 0);
        REQUIRE(prog.getBaseAddressLabel(2821).compare("bucket_acc") == 0);
        REQUIRE(prog.getBaseAddressLabel(2822).compare("sum") == 0);
      }
    }
  }
}
