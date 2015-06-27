include("${ALADDIN_SCRIPT_DIR}/buildAladdinTest.cmake")

# collect all the directory containing CMakeLists.txt
FILE(GLOB TEST_DIR */CMakeLists.txt)
foreach(sub_cmakelist ${TEST_DIR})
  get_filename_component(temp_dir ${sub_cmakelist} PATH)
  add_subdirectory(${temp_dir})
endforeach(sub_cmakelist)
