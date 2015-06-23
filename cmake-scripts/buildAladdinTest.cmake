set(py_script_dir "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
set(config_py "${py_script_dir}/config.py")
set(run_aladdin_py "${py_script_dir}/run_aladdin.py")
set(run_script_template "${CMAKE_SOURCE_DIR}/cmake-scripts/test_script.sh.in")
set(ALADDIN_EXECUTABLE "${CMAKE_BINARY_DIR}/common/aladdin")

# API example :
# build_aladdin_test(${TEST_NAME} SRC ${WORKLOAD} CONFIGS)
function(build_aladdin_test f_TEST_NAME f_SRC f_WORKLOAD f_CONFIGS)
  set(PROFILE_EXE "${CMAKE_CURRENT_BINARY_DIR}/${f_TEST_NAME}-instrumented")
  set(TRACE_DATA "${CMAKE_CURRENT_BINARY_DIR}/dynamic_trace.gz")

  set(f_CONFIG_ARRAY "${${f_CONFIGS}}")
  build_tracer_bitcode(${f_TEST_NAME} ${f_SRC} ${f_WORKLOAD})

  # Configure scripts for running tests of certain parameter
  # The scripts checks whether dynamic_trace exists. If not, run PROFILE_EXE
  # These scripts should be executable by users and ctest.
  foreach(f_CONFIG ${f_CONFIG_ARRAY})
    string(REPLACE " " ";" f_CONFIG_LIST "${f_CONFIG}")
    list(GET f_CONFIG_LIST 0 PART_CONFIG)
    list(GET f_CONFIG_LIST 1 UNROLL_CONFIG)
    list(GET f_CONFIG_LIST 2 PIPE_CONFIG)
    list(GET f_CONFIG_LIST 3 CLOCK_CONFIG)
                           
    set(PARAEMETER_STRING "p${PART_CONFIG}_u${UNROLL_CONFIG}_P${PIPE_CONFIG}_${CLOCK_CONFIG}ns")
    set(RUN_TEST_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/sim/${PARAEMETER_STRING}")
    set(CONFIG_FILE "${RUN_TEST_DIRECTORY}/config_${PARAEMETER_STRING}")
    set(run_script "${CMAKE_CURRENT_BINARY_DIR}/${f_TEST_NAME}_${PARAEMETER_STRING}.sh")

    configure_file(${run_script_template} ${run_script})
    set(f_ALADDIN_TEST_NAME ${f_TEST_NAME}_${PARAEMETER_STRING})
    add_test(NAME ${f_ALADDIN_TEST_NAME} COMMAND bash ${run_script})
    set_tests_properties(${f_ALADDIN_TEST_NAME} PROPERTIES DEPENDS ${f_TEST_NAME})
  endforeach(f_CONFIG)

  # add the scripts as one of ctest
endfunction(build_aladdin_test)
