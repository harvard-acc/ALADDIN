#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

SCENARIO("Test memory ambiguation with independent stores", "[mem_amb_indep]") {
  std::string trace_file("inputs/memory_ambiguation_trace.gz");
  std::string config_file("inputs/config-memory-ambiguation");

  /* Code:
   *   void test_independent(int* input0, int *input1, int *result, int rows, int cols) {
   *     int (*_result)[rows] = (int(*)[rows])result;
   *
   *     outer:
   *     for (int i = 0; i < rows; i++) {
   *       inner:
   *       for (int j = 0; j < cols; j++) {
   *         _result[i][j] = input0[j] + input1[j];
   *       }
   *     }
   *   }
   */
  GIVEN("A doubly nested loop where all stores go to independent addresses") {
    std::string bench("outputs/memory-ambiguation-indep");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    acc->globalOptimizationPass();
    // Two outer loop iterations worth of stores.
    std::vector<unsigned> stores{ 19,  32,  45,  58,  71,  84,  97,  110,
                                  131, 144, 157, 170, 183, 196, 209, 222 };
    WHEN("After memory ambiguation") {
      THEN("Stores should not have any dependences") {
        for (unsigned i = 0; i < stores.size() - 1; i++) {
          REQUIRE(acc->getNodeFromNodeId(stores[i])->is_store_op());
          REQUIRE(acc->getNodeFromNodeId(stores[i+1])->is_store_op());
          REQUIRE_FALSE(acc->doesEdgeExist(stores[i], stores[i+1]));
        }
      }
    }
  }

  /* Code:
   *   void test_indirect(int* input0, int *input1, int *result, int rows, int cols) {
   *     int (*_result)[rows] = (int(*)[rows])result;
   *
   *     outer:
   *     for (int i = 0; i < rows; i++) {
   *       inner:
   *       for (int j = 0; j < cols; j++) {
   *         _result[input0[j]][j] = input0[j] + input1[j];
   *       }
   *     }
   *   }
   */
  GIVEN("A doubly nested loop where all stores addresses are data dependent") {
    std::string bench("outputs/memory-ambiguation-dep");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    acc->buildDddg();
    acc->globalOptimizationPass();
    // Two outer loop iterations worth of stores.
    std::vector<unsigned> stores{ 1146, 1161, 1176, 1191, 1206, 1221,
                                  1236, 1251, 1271, 1286, 1301, 1316,
                                  1331, 1346, 1361, 1376 };
    WHEN("After memory ambiguation") {
      THEN("All stores should be serialized") {
        for (unsigned i = 0; i < stores.size() - 1; i++) {
          REQUIRE(acc->getNodeFromNodeId(stores[i])->is_store_op());
          REQUIRE(acc->getNodeFromNodeId(stores[i+1])->is_store_op());
          REQUIRE(acc->doesEdgeExist(stores[i], stores[i+1]));
        }
      }
    }
  }
}
