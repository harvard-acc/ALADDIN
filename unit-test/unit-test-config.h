#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "DDDG.h"
#include "file_func.h"
#include "Scratchpad.h"
#include "ScratchpadDatapath.h"

#ifndef INPUT_ROOT_DIR
  #define INPUT_ROOT_DIR .
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

const std::string root_dir = std::string(TOSTRING(INPUT_ROOT_DIR)) + "/";
