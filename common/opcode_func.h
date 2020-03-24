#include "power_func.h"

#ifndef OPCODE_FUNC_H
#define OPCODE_FUNC_H

#ifndef LLVM_VERSION
#error "Must define LLVM_VERSION!"
#elif LLVM_VERSION == 34
#include "opcode_func_llvm34.h"
#elif LLVM_VERSION == 60
#include "opcode_func_llvm60.h"
#else
#error "Only LLVM-3.4 and LLVM-6.0 are supported!"
#endif

// Returns the name of the opcode.
std::string opcode_name(uint8_t opcode);

#endif
