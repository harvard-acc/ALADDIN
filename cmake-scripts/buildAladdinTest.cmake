set(py_script_dir "${CMAKE_SOURCE_DIR}/SHOC/scripts")
set(config_py "${py_script_dir}/config.py")
set(run_aladdin_py "${py_script_dir}/run_aladdin.py")
set(gen_space_py "${py_script_dir}/plot_space.py")
set(run_script_template "${ALADDIN_SCRIPT_DIR}/test_script.sh.in")
set(plot_script_template "${ALADDIN_SCRIPT_DIR}/plot_config_space.sh.in")
set(ALADDIN_PLOT_TEST_NAME data_plot)


if(${BUILD_ON_SOURCE})
  set(ALADDIN_EXECUTABLE "${CMAKE_SOURCE_DIR}/common/aladdin")
else()
  set(ALADDIN_EXECUTABLE "${CMAKE_BINARY_DIR}/common/aladdin")
endif()

# API example :
# build_aladdin_test(${TEST_NAME} SRC ${WORKLOAD} CONFIGS)
function(build_aladdin_test f_TEST_NAME f_SRC f_WORKLOAD f_CONFIGS)
  set(PROFILE_EXE "${CMAKE_CURRENT_BINARY_DIR}/${f_TEST_NAME}-instrumented")
  set(TRACE_DATA "${CMAKE_CURRENT_BINARY_DIR}/dynamic_trace.gz")

  set(f_CONFIG_ARRAY "${${f_CONFIGS}}")
  build_tracer_bitcode(${f_TEST_NAME} ${f_SRC} ${f_WORKLOAD})

  # The python scripts is not thread-safe and want to create
  # this directory. Create it in advance.
  set(CREATE_DIR_LIST "${CMAKE_CURRENT_BINARY_DIR}/sim" "${py_script_dir}/data")
  foreach(TEST_OUTPUT_DIR ${CREATE_DIR_LIST})
    if(NOT EXISTS ${TEST_OUTPUT_DIR})
      file(MAKE_DIRECTORY ${TEST_OUTPUT_DIR})
    endif()
  endforeach(TEST_OUTPUT_DIR)


  set(run_script "${CMAKE_CURRENT_BINARY_DIR}/${f_TEST_NAME}.sh")
  configure_file(${run_script_template} ${run_script} @ONLY)

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

    set(f_ALADDIN_TEST_NAME ${f_TEST_NAME}_${PARAEMETER_STRING})
    add_test(NAME ${f_ALADDIN_TEST_NAME} COMMAND bash ${run_script} ${f_CONFIG_LIST})
    set_tests_properties(${f_ALADDIN_TEST_NAME} PROPERTIES DEPENDS ${f_TEST_NAME})

    list(APPEND ALADDIN_LIST ${f_ALADDIN_TEST_NAME})
  endforeach(f_CONFIG)

  set(plot_script "${CMAKE_CURRENT_BINARY_DIR}/${f_TEST_NAME}_plot.sh")
  configure_file(${plot_script_template} ${plot_script} @ONLY)
  set(f_plot_test_name ${f_TEST_NAME}_${ALADDIN_PLOT_TEST_NAME})
  add_test(NAME ${f_plot_test_name} COMMAND bash ${plot_script})

  set_tests_properties(${f_plot_test_name} PROPERTIES DEPENDS "${ALADDIN_LIST}")

endfunction(build_aladdin_test)
