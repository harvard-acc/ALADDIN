#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

/* Code:
 *
 * float top_level(int a, int b) {
 *   int ret_val = 0;
 *
 *   loop:
 *   for (int i = 0; i < b; i++) {
 *     ret_val += (sqrt(a) + exp(a) + log(a));
 *   }
 *   return ret_val;
 * }
 *
 * int main() {
 *   float ret_val = top_level(9, 2);
 *   printf("Result = %f\n", ret_val);
 *   return 0;
 * }
 *
 */

SCENARIO("Test special math operations", "[special_math_op]") {
  GIVEN("C math library function sqrt()") {
    std::string bench("outputs/special_math_op");
    std::string trace_file("inputs/special-math-op-trace.gz");
    std::string config_file("inputs/config-special-math-op");

    ScratchpadDatapath* acc;
    Scratchpad* spad;
    acc = new ScratchpadDatapath(bench, trace_file, config_file);
    auto& prog = acc->getProgram();
    acc->buildDddg();
    acc->globalOptimizationPass();
    acc->prepareForScheduling();

    WHEN("After scheduling") {
        REQUIRE(prog.nodes.at(6)->get_microop_name() ==
                "SpecialMathOp");
        REQUIRE(prog.nodes.at(6)->get_special_math_op() ==
                "sqrt");
        REQUIRE(prog.nodes.at(7)->get_microop_name() ==
                "SpecialMathOp");
        REQUIRE(prog.nodes.at(7)->get_special_math_op() ==
                "exp");
        REQUIRE(prog.nodes.at(9)->get_microop_name() ==
                "SpecialMathOp");
        REQUIRE(prog.nodes.at(9)->get_special_math_op() ==
                "log");
        REQUIRE(prog.nodes.at(19)->get_microop_name() ==
                "SpecialMathOp");
        REQUIRE(prog.nodes.at(19)->get_special_math_op() ==
                "sqrt");
        REQUIRE(prog.nodes.at(20)->get_microop_name() ==
                "SpecialMathOp");
        REQUIRE(prog.nodes.at(20)->get_special_math_op() ==
                "exp");
        REQUIRE(prog.nodes.at(22)->get_microop_name() ==
                "SpecialMathOp");
        REQUIRE(prog.nodes.at(22)->get_special_math_op() ==
                "log");
    }
    delete acc;
  }
}
