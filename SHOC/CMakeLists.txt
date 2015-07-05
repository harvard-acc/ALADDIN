include("${ALADDIN_SCRIPT_DIR}/buildAladdinTest.cmake")
include("${ALADDIN_SCRIPT_DIR}/gen_space.cmake")
generate_space(ALL_CONFIG_SPACE)

# collect all the directory containing CMakeLists.txt
FILE(GLOB TEST_DIR */CMakeLists.txt)
foreach(sub_cmakelist ${TEST_DIR})
  get_filename_component(temp_dir ${sub_cmakelist} PATH)
  add_subdirectory(${temp_dir})
endforeach(sub_cmakelist)

# add a test for plotting the design space
set(plot_script "${CMAKE_BINARY_DIR}/SHOC/PLOT.sh")
configure_file(${plot_script_template} ${plot_script})
add_test(NAME ${ALADDIN_PLOT_TEST_NAME} COMMAND bash ${plot_script})

# This test needs to be the last executabled one.
set_tests_properties(${ALADDIN_PLOT_TEST_NAME} PROPERTIES DEPENDS "${ALADDIN_LIST}")
