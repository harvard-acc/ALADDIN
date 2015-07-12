cmake_minimum_required(VERSION 2.8.12)

# disable in-source-build
get_filename_component(srcdir "${CMAKE_SOURCE_DIR}" REALPATH)
get_filename_component(bindir "${CMAKE_BINARY_DIR}" REALPATH)
if("${srcdir}" STREQUAL "${bindir}")
  message(FATAL_ERROR "In-source builds are forbidden!")
endif()

project(Aladdin CXX)
enable_testing()

set(ALADDIN_SCRIPT_DIR "${CMAKE_SOURCE_DIR}/cmake-scripts")
include("${ALADDIN_SCRIPT_DIR}/AladdinConfig.cmake")
include("${ALADDIN_SCRIPT_DIR}/checkDependency.cmake")


# reuse cmake-script of LLVM-Tracer
set(SCRIPT_DIR "${TRACER_DIR}/cmake-scripts")
include("${SCRIPT_DIR}/TracerConfig.cmake")
include("${SCRIPT_DIR}/findAndSetLLVM.cmake")

# build LLVM-Tracer
add_subdirectory(${TRACER_DIR} "${CMAKE_BINARY_DIR}/LLVM-Tracer")

# build aladdin
add_subdirectory(common)
add_subdirectory(SHOC)
add_subdirectory(unit-test)
