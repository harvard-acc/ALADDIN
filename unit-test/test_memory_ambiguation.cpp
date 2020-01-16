#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test memory ambiguation with independent stores", "[mem_amb]") {
  // This trace contains two top level functions: the first is test_independent,
  // and the second is test_indirect. A call to buildDddg will read in one top
  // level entry.
  std::string trace_file("inputs/memory_ambiguation_trace.gz");
  std::string config_file("inputs/config-memory-ambiguation");

  ScratchpadDatapath* acc;
  Scratchpad* spad;
  acc = new ScratchpadDatapath(
      "outputs/memory-ambiguation", trace_file, config_file);
  auto& prog = acc->getProgram();
  // This builds DDDG for test_independent.
  acc->buildDddg();
  acc->globalOptimizationPass();

  GIVEN("A doubly nested loop where all stores go to independent addresses") {
    // Two outer loop iterations worth of stores.
    std::vector<unsigned> stores{ 19,  30,  41,  52,  63,  74,  85,  96,
                                  115, 126, 137, 148, 159, 170, 181, 192 };
    THEN("Stores should not have any dependences") {
      for (unsigned i = 0; i < stores.size() - 1; i++) {
        REQUIRE(prog.nodes.at(stores[i])->is_store_op());
        REQUIRE(prog.nodes.at(stores[i + 1])->is_store_op());
        REQUIRE_FALSE(prog.edgeExists(stores[i], stores[i + 1]));
      }
    }
  }

  GIVEN("A doubly nested loop where all stores addresses are data dependent") {
    // Clear the datapath for the next top level entry.
    acc->clearDatapath();
    // This builds DDDG for test_indirect.
    acc->buildDddg();
    acc->globalOptimizationPass();
    // Two outer loop iterations worth of stores.
    std::vector<unsigned> stores{ 19,  33,  47,  61,  75,  89,  103, 117,
                                  137, 151, 165, 179, 193, 207, 221, 235 };
    THEN("All stores should be serialized") {
      for (unsigned i = 0; i < stores.size() - 1; i++) {
        REQUIRE(prog.nodes.at(stores[i])->is_store_op());
        REQUIRE(prog.nodes.at(stores[i + 1])->is_store_op());
        REQUIRE(prog.edgeExists(stores[i], stores[i + 1]));
      }
    }
  }
}
