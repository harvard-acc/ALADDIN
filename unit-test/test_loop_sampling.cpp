#include "catch.hpp"
#include "ScratchpadDatapath.h"

void scheduleDatapath(ScratchpadDatapath* acc) {
  acc->buildDddg();
  acc->globalOptimizationPass();
  acc->prepareForScheduling();
  // Scheduling
  while (!acc->step()) {
  }
  acc->dumpStats();
}

void clearDatapath(ScratchpadDatapath* acc) {
  acc->clearDatapath();
}

SCENARIO("Test sampling on loops", "[loop_sampling]") {
  GIVEN("Single loop") {
    /* Code:
     *
     * int top_level(int a[10], int loop_size, int sample_size) {
     *   int ret_val = 0;
     *   setSamplingFactor("loop", loop_size / sample_size);
     *   loop:
     *   for (int i = 0; i < sample_size; i++) {
     *       ret_val += a[i];
     *   }
     *   return ret_val;
     * }
     *
     * int main() {
     *   int a[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
     *   int result = top_level(a, 10, 1);
     *   printf("result = %d\n", result);
     *   return 0;
     * }
     *
     */
    WHEN("Sampled loop is not unrolled") {
      std::string bench("outputs/loop-sampling");
      std::string trace_file("inputs/loop-sampling-single-loop-trace.gz");
      std::string trace_ref_file(
          "inputs/loop-sampling-single-loop-ref-trace.gz");
      std::string config_file("inputs/config-loop-sampling");

      ScratchpadDatapath* acc =
          new ScratchpadDatapath(bench, trace_file, config_file);
      scheduleDatapath(acc);

      // The reference accelerator doesn't do any sampling.
      ScratchpadDatapath* acc_ref =
          new ScratchpadDatapath(bench, trace_ref_file, config_file);
      scheduleDatapath(acc_ref);

      WHEN("After scheduling") {
        REQUIRE(acc->getCurrentCycle() == acc_ref->getCurrentCycle());
      }
    }

    WHEN("Sampled loop has unrolling factor 2") {
      // This changes sample_size to 4 and "loop" is unrolled by factor 2.
      std::string bench("outputs/loop-sampling");
      std::string trace_file("inputs/loop-sampling-unrolling-trace.gz");
      std::string trace_ref_file(
          "inputs/loop-sampling-single-loop-ref-trace.gz");
      std::string config_file("inputs/config-loop-sampling-unrolling");

      ScratchpadDatapath* acc =
          new ScratchpadDatapath(bench, trace_file, config_file);
      scheduleDatapath(acc);

      // The reference accelerator doesn't do any sampling.
      ScratchpadDatapath* acc_ref =
          new ScratchpadDatapath(bench, trace_ref_file, config_file);
      scheduleDatapath(acc_ref);

      WHEN("After scheduling") {
        REQUIRE(acc->getCurrentCycle() == acc_ref->getCurrentCycle());
      }
    }
  }

  GIVEN("Nested loops") {
    /* Code:
     *
     * int top_level(int a[10], int loop_size, int sample_size) {
     *   int ret_val = 0;
     *   setSamplingFactor("loop", loop_size / sample_size);
     *   loop:
     *   for (int i = 0; i < sample_size; i++) {
     *     ret_val += a[i];
     *     loop_inner:
     *     for (int j = 0; j < loop_size; j++) {
     *       ret_val += a[j];
     *     }
     *   }
     *   return ret_val;
     * }
     *
     * int main() {
     *   int a[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
     *   int result = top_level(a, 10, 1);
     *   printf("result = %d\n", result);
     *   return 0;
     * }
     *
     */
    std::string bench("outputs/loop-sampling");
    std::string trace_file("inputs/loop-sampling-nested-trace.gz");
    std::string trace_ref_file("inputs/loop-sampling-nested-ref-trace.gz");
    std::string config_file("inputs/config-loop-sampling");

    ScratchpadDatapath* acc =
        new ScratchpadDatapath(bench, trace_file, config_file);
    scheduleDatapath(acc);

    // The reference accelerator doesn't do any sampling.
    ScratchpadDatapath* acc_ref =
        new ScratchpadDatapath(bench, trace_ref_file, config_file);
    scheduleDatapath(acc_ref);

    WHEN("After scheduling") {
      REQUIRE(acc->getCurrentCycle() == acc_ref->getCurrentCycle());
    }
  }

  GIVEN("Multiple loops") {
    /* Code:
     *
     * int top_level(int a[10], int loop_size, int sample_size) {
     *   int ret_val = 0;
     *   setSamplingFactor("loop", loop_size / sample_size);
     *   loop:
     *   for (int i = 0; i < sample_size; i++) {
     *     ret_val += a[i];
     *   }
     *
     *   loop1:
     *   for (int i = 0; i < loop_size; i++) {
     *     ret_val += a[i];
     *   }
     *   return ret_val;
     * }
     *
     * int main() {
     *   int a[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
     *   int result = top_level(a, 10, 1);
     *   printf("result = %d\n", result);
     *   return 0;
     * }
     *
     */
    std::string bench("outputs/loop-sampling");
    std::string trace_file("inputs/loop-sampling-multiple-loops-trace.gz");
    std::string trace_ref_file(
        "inputs/loop-sampling-multiple-loops-ref-trace.gz");
    std::string config_file("inputs/config-loop-sampling");

    ScratchpadDatapath* acc =
        new ScratchpadDatapath(bench, trace_file, config_file);
    scheduleDatapath(acc);

    // The reference accelerator doesn't do any sampling.
    ScratchpadDatapath* acc_ref =
        new ScratchpadDatapath(bench, trace_ref_file, config_file);
    scheduleDatapath(acc_ref);

    WHEN("After scheduling") {
      REQUIRE(acc->getCurrentCycle() == acc_ref->getCurrentCycle());
    }
  }

  GIVEN("Multiple invocations with different sampling factors") {
    /* Code:
     *
     * int top_level(int a[10], int loop_size, int sample_size) {
     *   int ret_val = 0;
     *   setSamplingFactor("loop", loop_size / sample_size);
     *   loop:
     *   for (int i = 0; i < sample_size; i++) {
     *     ret_val += a[i];
     *     loop_inner:
     *     for (int j = 0; j < loop_size; j++) {
     *       ret_val += a[j];
     *     }
     *   }
     *   return ret_val;
     * }
     *
     * int main() {
     *   int a[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
     *   int result0 = top_level(a, 10, 1);
     *   int result1 = top_level(a, 10, 2);
     *   printf("result0 = %d, result1 = %d\n", result0, result1);
     *   return 0;
     * }
     *
     */
    std::string bench("outputs/loop-sampling");
    std::string trace_file("inputs/loop-sampling-multiple-invoc-trace.gz");
    std::string trace_ref_file(
        "inputs/loop-sampling-multiple-invoc-ref-trace.gz");
    std::string config_file("inputs/config-loop-sampling");

    ScratchpadDatapath* acc =
        new ScratchpadDatapath(bench, trace_file, config_file);
    scheduleDatapath(acc);
    int first_invoc_cycle = acc->getCurrentCycle();
    acc->clearDatapath();
    scheduleDatapath(acc);
    int second_invoc_cycle = acc->getCurrentCycle();

    // The reference accelerator doesn't do any sampling.
    ScratchpadDatapath* acc_ref =
        new ScratchpadDatapath(bench, trace_ref_file, config_file);
    scheduleDatapath(acc_ref);
    int first_invoc_ref_cycle = acc_ref->getCurrentCycle();
    acc_ref->clearDatapath();
    scheduleDatapath(acc_ref);
    int second_invoc_ref_cycle = acc_ref->getCurrentCycle();

    WHEN("After scheduling") {
      REQUIRE(first_invoc_cycle == first_invoc_ref_cycle);
      REQUIRE(second_invoc_cycle == second_invoc_ref_cycle);
    }
  }

  GIVEN("Inner loops") {
    /* Code:
     *
     * int top_level(int a[10], int loop_size, int sample_size) {
     *   int ret_val = 0;
     *   setSamplingFactor("loop0", loop_size / sample_size);
     *   setSamplingFactor("loop1", loop_size / sample_size);
     *   setSamplingFactor("loop2", 1);
     *   setSamplingFactor("loop3", 1);
     *   loop0:
     *   for (int i = 0; i < sample_size; i++) {
     *       ret_val += a[i];
     *       loop1:
     *       for (int j = 0; j < sample_size; j++)
     *           ret_val += a[j];
     *       loop2:
     *       for (int j = 0; j < loop_size; j++)
     *          ret_val *= a[j];
     *   }
     *   loop3:
     *   for (int i = 0; i < inputs_size; i++)
     *       ret_val += a[i];
     *   return ret_val;
     * }
     *
     * int main() {
     *   int a[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
     *   int result = top_level(a, 10, 1);
     *   printf("result = %d\n", result);
     *   return 0;
     * }
     *
     */
    std::string bench("outputs/loop-sampling");
    std::string trace_file("inputs/loop-sampling-inner-loops-trace.gz");
    std::string trace_ref_file("inputs/loop-sampling-inner-loops-ref-trace.gz");
    WHEN("Loop pipelining is not used") {
      std::string config_file("inputs/config-loop-sampling-inner");

      ScratchpadDatapath* acc =
          new ScratchpadDatapath(bench, trace_file, config_file);
      scheduleDatapath(acc);

      // The reference accelerator doesn't do any sampling.
      ScratchpadDatapath* acc_ref =
          new ScratchpadDatapath(bench, trace_ref_file, config_file);
      scheduleDatapath(acc_ref);

      WHEN("After scheduling") {
        REQUIRE(acc->getCurrentCycle() == acc_ref->getCurrentCycle());
      }
    }

    WHEN("Loop pipelining is used") {
      // Sampling is not supposed to work with pipelined loops, but it should
      // not affect the upsampling on other loops.
      // In the code, loop2 will be pipelined.
      std::string config_file("inputs/config-loop-sampling-inner-pipelining");
      ScratchpadDatapath* acc =
          new ScratchpadDatapath(bench, trace_file, config_file);
      scheduleDatapath(acc);

      // The reference accelerator doesn't do any sampling.
      ScratchpadDatapath* acc_ref =
          new ScratchpadDatapath(bench, trace_ref_file, config_file);
      scheduleDatapath(acc_ref);

      WHEN("After scheduling") {
        REQUIRE(acc->getCurrentCycle() == acc_ref->getCurrentCycle());
      }
    }
  }
  GIVEN("Sampling pipelined loops") {
    WHEN("Single pipelined loop") {
      /* Code:
       *
       * "loop" is pipelined.
       *
       * void top_level(int* a, int* b, int loop_size, int sample_size) {
       *   setSamplingFactor("loop", loop_size / sample_size);
       *   loop:
       *   for (int i = 0; i < sample_size; i++) {
       *       b[i] += a[i] * a[i] + i * i * 1.0;
       *   }
       * }
       *
       * int main() {
       *   int a[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
       *   int b[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
       *   top_level(a, b, 10, 2);
       *   for (int i = 0; i < 10; i++)
       *     printf("%d, ", b[i]);
       *   printf("\n");
       *   return 0;
       * }
       *
       */
      std::string bench("outputs/loop-sampling");
      std::string trace_file("inputs/loop-sampling-single-pipelined-trace.gz");
      std::string trace_ref_file(
          "inputs/loop-sampling-single-pipelined-ref-trace.gz");
      std::string config_file("inputs/config-loop-sampling-pipelined");

      ScratchpadDatapath* acc =
          new ScratchpadDatapath(bench, trace_file, config_file);
      scheduleDatapath(acc);

      // The reference accelerator doesn't do any sampling.
      ScratchpadDatapath* acc_ref =
          new ScratchpadDatapath(bench, trace_ref_file, config_file);
      scheduleDatapath(acc_ref);

      WHEN("After scheduling") {
        REQUIRE(acc->getCurrentCycle() == acc_ref->getCurrentCycle());
      }
    }
    WHEN("With flattened inner loop") {
      /* Code:
       *
       * "loop" is pipelined, "loop_flatten" is flattened.
       *
       * void top_level(int* a, int* b, int loop_size, int sample_size) {
       *   setSamplingFactor("loop", loop_size / sample_size);
       *   loop:
       *   for (int i = 0; i < sample_size; i++) {
       *     b[i] += a[i];
       *     loop_flatten:
       *     for (int j = 0; j < loop_size; j++)
       *       b[i] += j * j * 1.0;
       *   }
       * }
       *
       * int main() {
       *   int a[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
       *   int b[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
       *   top_level(a, b, 10, 2);
       *   for (int i = 0; i < 10; i++)
       *     printf("%d, ", b[i]);
       *   printf("\n");
       *   return 0;
       * }
       *
       */
      std::string bench("outputs/loop-sampling");
      std::string trace_file(
          "inputs/loop-sampling-pipelined-flattened-trace.gz");
      std::string trace_ref_file(
          "inputs/loop-sampling-pipelined-flattened-ref-trace.gz");
      std::string config_file("inputs/config-loop-sampling-pipelined");

      ScratchpadDatapath* acc =
          new ScratchpadDatapath(bench, trace_file, config_file);
      scheduleDatapath(acc);

      // The reference accelerator doesn't do any sampling.
      ScratchpadDatapath* acc_ref =
          new ScratchpadDatapath(bench, trace_ref_file, config_file);
      scheduleDatapath(acc_ref);

      WHEN("After scheduling") {
        REQUIRE(acc->getCurrentCycle() == acc_ref->getCurrentCycle());
      }
    }
    WHEN("Inner pipelined loop") {
      /* Code:
       *
       * "loop1" is pipelined.
       *
       * void top_level(int* a, int* b, int loop_size, int sample_size) {
       *   setSamplingFactor("loop0", loop_size / sample_size);
       *   setSamplingFactor("loop1", loop_size / sample_size);
       *   loop0:
       *   for (int i = 0; i < sample_size; i++) {
       *     b[i] += a[i];
       *     loop1:
       *     for (int j = 0; j < sample_size; j++) {
       *         b[i] += a[j] * a[j] + j * j * 1.0;
       *     }
       *   }
       * }
       *
       * int main() {
       *   int a[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
       *   int b[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
       *   top_level(a, b, 10, 2);
       *   for (int i = 0; i < 10; i++)
       *     printf("%d, ", b[i]);
       *   printf("\n");
       *   return 0;
       * }
       *
       */
      std::string bench("outputs/loop-sampling");
      std::string trace_file("inputs/loop-sampling-inner-pipelined-trace.gz");
      std::string trace_ref_file(
          "inputs/loop-sampling-inner-pipelined-ref-trace.gz");
      std::string config_file("inputs/config-loop-sampling-pipelined");

      ScratchpadDatapath* acc =
          new ScratchpadDatapath(bench, trace_file, config_file);
      scheduleDatapath(acc);

      // The reference accelerator doesn't do any sampling.
      ScratchpadDatapath* acc_ref =
          new ScratchpadDatapath(bench, trace_ref_file, config_file);
      scheduleDatapath(acc_ref);

      WHEN("After scheduling") {
        REQUIRE(acc->getCurrentCycle() == acc_ref->getCurrentCycle());
      }
    }
  }
}
