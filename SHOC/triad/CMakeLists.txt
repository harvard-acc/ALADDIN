set(TEST_NAME triad)

# Source codes
set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}.c")

# The workload string used by LLVM-Tracer
set(WORKLOAD "triad")

# If want to use your own config space for ctest,
# write somethin like the following.
# set(CONFIGS "2 2 1 6" "4 4 2 3")

set(CONFIGS ${ALL_CONFIG_SPACE})

build_aladdin_test(${TEST_NAME} SRC ${WORKLOAD} CONFIGS)

