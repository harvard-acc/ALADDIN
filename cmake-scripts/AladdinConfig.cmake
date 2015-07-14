set(HAS_ALADDIN TRUE)

# settings for LLVM-Tracer
if(NOT DEFINED BUILD_ON_SOURCE)
  set(BUILD_ON_SOURCE FALSE)
endif()

# Check whether LLVM-Tracer exists
if(DEFINED TRACER_DIR)
  get_filename_component(TRACER_DIR ${TRACER_DIR} ABSOLUTE)
elseif("$ENV{TRACER_HOME}" STREQUAL "")
  set(TRACER_DIR "${CMAKE_SOURCE_DIR}/LLVM-Tracer")
else()
  set(TRACER_DIR "$ENV{TRACER_HOME}")
endif()


if(NOT EXISTS ${TRACER_DIR})
  message(FATAL_ERROR "finds no ${TRACER_DIR}")
endif()

if(NOT DEFINED BOOST_ROOT)
  set(BOOST_ROOT "$ENV{BOOST_ROOT}")
endif()
