#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"
#include "SourceManager.h"

SCENARIO("Test Dynamic Method Name w/ aes", "[aes]") {
  GIVEN("Test MachSuite aes-aes") {
    std::string bench("outputs/aes-aes");
    std::string trace_file("inputs/aes-aes-trace.gz");
    std::string config_file("inputs/config-aes-aes");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    WHEN("DDDG is generated.") {
      acc = new ScratchpadDatapath(bench, trace_file, config_file);
      acc->buildDddg();
      SourceManager& srcManager = acc->get_source_manager();

      THEN("The unique static node id should match.") {
        REQUIRE(acc->getNodeFromNodeId(0)->get_dynamic_instruction().str(
                    srcManager) == "aes256_encrypt_ecb-0-0-4");
        REQUIRE(acc->getNodeFromNodeId(325)->get_dynamic_instruction().str(
                    srcManager) == "aes256_encrypt_ecb-0-7-0");
        REQUIRE(acc->getNodeFromNodeId(326)->get_dynamic_instruction().str(
                    srcManager) == "aes_expandEncKey-0-1");
        REQUIRE(acc->getNodeFromNodeId(664)->get_dynamic_instruction().str(
                    srcManager) == "aes256_encrypt_ecb-0-7-0");
        REQUIRE(acc->getNodeFromNodeId(665)->get_dynamic_instruction().str(
                    srcManager) == "aes_expandEncKey-1-1");
        REQUIRE(acc->getNodeFromNodeId(2699)->get_dynamic_instruction().str(
                    srcManager) == "aes256_encrypt_ecb-0-9-0");
        REQUIRE(acc->getNodeFromNodeId(2700)->get_dynamic_instruction().str(
                    srcManager) == "aes_addRoundKey_cpy-0-0-5");
      }
    }
  }
}
