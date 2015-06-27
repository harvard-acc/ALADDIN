FILE(GLOB ALADDIN_SRC "*.cpp")

set(CACTI_DIR "cacti-p")
FILE(GLOB CACTI_SRC "${CACTI_DIR}/*.cc")

if (NOT DEFINED NTHREADS)
  set(NTHREADS 8)
endif()

# find Boost library
find_package(Boost 1.55.0 REQUIRED COMPONENTS graph regex)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${ZLIB_INCLUDE_DIRS})

add_definitions("-DNTHREADS=${NTHREADS}")
include_directories("${CACTI_DIR}")

add_library(cacit-p ${CACTI_SRC})
add_executable(aladdin ${ALADDIN_SRC})
target_link_libraries(aladdin "cacit-p" ${Boost_LIBRARIES} pthread z)

if(${BUILD_ON_SOURCE})
  set_target_properties(aladdin PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                         "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
