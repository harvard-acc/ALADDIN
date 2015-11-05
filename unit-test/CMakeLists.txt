include_directories(${ZLIB_INCLUDE_DIRS})
include_directories("${CMAKE_SOURCE_DIR}/common")
add_definitions("-DINPUT_ROOT_DIR=${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB TEST_SRC_LIST "*.cpp")

foreach(test_src ${TEST_SRC_LIST})
  get_filename_component(test_exe "${test_src}" NAME_WE)
  add_executable(${test_exe} ${test_src})
  target_link_libraries(${test_exe} aladdin_lib "cacit-p"
                         ${Boost_LIBRARIES} pthread z)

  set(TEST_NAME "unit_${test_exe}")
  add_test(NAME ${TEST_NAME} COMMAND $<TARGET_FILE:${test_exe}>)
endforeach(test_src)
