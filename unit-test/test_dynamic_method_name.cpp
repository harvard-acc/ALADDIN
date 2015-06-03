#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

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
      THEN("The unique static node id should match.") {
        REQUIRE(acc->getNodeFromNodeId(0)->get_static_node_id().compare(
                    "aes256_encrypt_ecb-0-0-4") == 0);
        REQUIRE(acc->getNodeFromNodeId(325)->get_static_node_id().compare(
                    "aes256_encrypt_ecb-0-7-0") == 0);
        REQUIRE(acc->getNodeFromNodeId(326)->get_static_node_id().compare(
                    "aes_expandEncKey-0-1") == 0);
        REQUIRE(acc->getNodeFromNodeId(664)->get_static_node_id().compare(
                    "aes256_encrypt_ecb-0-7-0") == 0);
        REQUIRE(acc->getNodeFromNodeId(665)->get_static_node_id().compare(
                    "aes_expandEncKey-1-1") == 0);
        REQUIRE(acc->getNodeFromNodeId(2699)->get_static_node_id().compare(
                    "aes256_encrypt_ecb-0-9-0") == 0);
        REQUIRE(acc->getNodeFromNodeId(2700)->get_static_node_id().compare(
                    "aes_addRoundKey_cpy-0-0-5") == 0);
      }
    }
  }
}
