list(APPEND CMAKE_PREFIX_PATH ${BOOST_ROOT})
find_package(Boost 1.55 REQUIRED COMPONENTS graph regex)

include_directories(${Boost_INCLUDE_DIRS})
