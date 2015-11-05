get_filename_component(TEST_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Source codes
file(GLOB SRC "*.c*")

# The workload string used by LLVM-Tracer
set(WORKLOAD "fft1D_512,step1,step2,step3,step4,step5,step6,step7,step8,step9,step10,step11")

# If want to use your own config space for ctest,
# write somethin like the following.
# set(CONFIGS "2 2 1 6" "4 4 2 3")

set(CONFIGS ${ALL_CONFIG_SPACE})

build_aladdin_test(${TEST_NAME} SRC ${WORKLOAD} CONFIGS)
