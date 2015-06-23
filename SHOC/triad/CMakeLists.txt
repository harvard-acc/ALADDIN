set(TEST_NAME triad)
set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}.c")
set(WORKLOAD "triad")
set(CONFIGS "2 2 1 6"
            "4 4 2 3")
build_aladdin_test(${TEST_NAME} SRC ${WORKLOAD} CONFIGS)

