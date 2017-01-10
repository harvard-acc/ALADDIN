#include <sys/time.h>
#include <iostream>
#include <sstream>

#include "DDDG.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"
#include "file_func.h"

#define RUN_TEST(TEST_NAME) \
  run_test(#TEST_NAME, TEST_NAME)

typedef int (*test_function)(void);

long run(test_function test) {
  struct timeval start, end;

  std::streambuf* old = std::cout.rdbuf();
  std::stringstream redirect;
  std::cout.rdbuf(redirect.rdbuf());

  gettimeofday(&start, NULL);
  test();
  gettimeofday(&end, NULL);

  std::cout.rdbuf(old);

  long diff = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec);
  return diff;
}

void run_test(const char* name, test_function test) {

  std::cout << name << " :\t";
  long elapsed = run(test);
  std::cout << elapsed << " usec.\n";
}


void execute(ScratchpadDatapath* acc) {
  acc->buildDddg();
  acc->globalOptimizationPass();
  acc->prepareForScheduling();
}

int triad_dma() {
  std::string bench("outputs/triad-dma");
  std::string trace_file("inputs/triad-dma-trace.gz");
  std::string config_file("inputs/config-triad-dma-p2-u2-P1");

  ScratchpadDatapath* acc =
      new ScratchpadDatapath(bench, trace_file, config_file);
  execute(acc);
  delete acc;
}

int reduction() {
  std::string bench("outputs/reduction-128");
  std::string trace_file("inputs/reduction-128-trace.gz");
  std::string config_file("inputs/config-reduction-p4-u4-P1");

  ScratchpadDatapath* acc =
      new ScratchpadDatapath(bench, trace_file, config_file);
  execute(acc);
  delete acc;
}

int pp_scan() {
  std::string bench("outputs/pp_scan-128");
  std::string trace_file("inputs/pp_scan-128-trace.gz");
  std::string config_file("inputs/config-pp_scan-p4-u4-P1");

  ScratchpadDatapath* acc =
      new ScratchpadDatapath(bench, trace_file, config_file);
  execute(acc);
  delete acc;
}

int aes() {
  std::string bench("outputs/aes-aes");
  std::string trace_file("inputs/aes-aes-trace.gz");
  std::string config_file("inputs/config-aes-aes");

  ScratchpadDatapath* acc =
      new ScratchpadDatapath(bench, trace_file, config_file);
  execute(acc);
  delete acc;
}

int main() {
  RUN_TEST(aes);
  RUN_TEST(pp_scan);
  RUN_TEST(reduction);
  RUN_TEST(triad_dma);

  return 0;

}
