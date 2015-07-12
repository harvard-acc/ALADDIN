#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

#ifndef INPUT_ROOT_DIR
  #define INPUT_ROOT_DIR .
#endif

// turn foo into "foo"
// reference: http://stackoverflow.com/questions/240353/convert-a-preprocessor-token-to-a-string
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

const std::string root_dir = std::string(TOSTRING(INPUT_ROOT_DIR)) + "/";
