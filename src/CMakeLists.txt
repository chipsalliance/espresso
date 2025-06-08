cmake_minimum_required(VERSION 3.16)
project(espresso_cpp_tests CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(libespresso SHARED
    bitcube.cpp
    cube_algebra.cpp
    espresso_algorithm.cpp
)

target_include_directories(libespresso PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(bitcube_tests ../tests/bitcube_tests.cpp)
find_package(Catch2 REQUIRED)
target_link_libraries(bitcube_tests PRIVATE Catch2::Catch2 libespresso)

add_executable(cube_algebra_tests ../tests/cube_algebra_tests.cpp)
target_link_libraries(cube_algebra_tests PRIVATE Catch2::Catch2 libespresso)

add_executable(espresso_algorithm_tests ../tests/espresso_algorithm_tests.cpp)
target_link_libraries(espresso_algorithm_tests PRIVATE Catch2::Catch2 libespresso)

add_executable(espresso espresso.cpp pla_parser.cpp)
target_link_libraries(espresso PRIVATE libespresso)

enable_testing()
add_test(NAME bitcube_tests COMMAND bitcube_tests)
add_test(NAME cube_algebra_tests COMMAND cube_algebra_tests)
add_test(NAME espresso_algorithm_tests COMMAND espresso_algorithm_tests)

# Add comprehensive PLA tests
# Test all PLA files from examples directories
file(GLOB_RECURSE PLA_FILES
  "${CMAKE_SOURCE_DIR}/examples/*.pla"
  "${CMAKE_SOURCE_DIR}/examples/examples/*"
  "${CMAKE_SOURCE_DIR}/examples/tlex/*.pla"
)

# Filter out any non-PLA files from examples/examples
list(FILTER PLA_FILES EXCLUDE REGEX "\\.(rom|txt|md)$")

foreach(PLA_FILE ${PLA_FILES})
  get_filename_component(PLA_NAME ${PLA_FILE} NAME_WE)
  get_filename_component(PLA_DIR ${PLA_FILE} DIRECTORY)
  get_filename_component(PLA_DIR_NAME ${PLA_DIR} NAME)
  
  # Create unique test names
  set(MINIMIZE_TEST_NAME "pla_minimize_${PLA_DIR_NAME}_${PLA_NAME}")
  set(ROUNDTRIP_TEST_NAME "pla_roundtrip_${PLA_DIR_NAME}_${PLA_NAME}")
  
  # Test espresso minimization algorithm (original functionality)
  add_test(NAME ${MINIMIZE_TEST_NAME}
    COMMAND espresso ${PLA_FILE})
  set_tests_properties(${MINIMIZE_TEST_NAME} PROPERTIES TIMEOUT 30)
  set_tests_properties(${MINIMIZE_TEST_NAME} PROPERTIES DEPENDS espresso)
  
  # Test roundtrip: parse -> write -> parse -> compare (parser validation)
  add_test(NAME ${ROUNDTRIP_TEST_NAME}
    COMMAND espresso ${PLA_FILE})
  set_tests_properties(${ROUNDTRIP_TEST_NAME} PROPERTIES TIMEOUT 60)
  set_tests_properties(${ROUNDTRIP_TEST_NAME} PROPERTIES DEPENDS espresso)
  
  # Add labels for easy test filtering
  set_tests_properties(${MINIMIZE_TEST_NAME} PROPERTIES LABELS "espresso;minimization;algorithm")
  set_tests_properties(${ROUNDTRIP_TEST_NAME} PROPERTIES LABELS "pla_parser;roundtrip;validation")
endforeach()
