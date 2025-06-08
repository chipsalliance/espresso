cmake_minimum_required(VERSION 3.16)

# Legacy C implementation of Espresso
project(espresso_legacy C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Enable strict warnings and sanitizers by default
option(ESPRESSO_LEGACY_SANITIZE "Enable AddressSanitizer and UBSan for legacy build" ON)
if(ESPRESSO_LEGACY_SANITIZE)
  set(SAN_FLAGS "-fsanitize=address,undefined" "-fno-omit-frame-pointer")
  add_compile_options(${SAN_FLAGS})
  add_link_options(${SAN_FLAGS})
endif()

# Legacy source files
file(GLOB LEGACY_SRC "*.c")

add_executable(espresso_legacy ${LEGACY_SRC})
set_property(TARGET espresso_legacy PROPERTY C_STANDARD 99)

# Install CLI for regression tests
include(GNUInstallDirs)
install(TARGETS espresso_legacy RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Enable testing
enable_testing()

# Add regression tests for all PLA files using legacy implementation
file(GLOB_RECURSE PLA_FILES
  "${CMAKE_SOURCE_DIR}/examples/*.pla"
  "${CMAKE_SOURCE_DIR}/examples/examples/*"
  "${CMAKE_SOURCE_DIR}/examples/hard_examples/*"
  "${CMAKE_SOURCE_DIR}/examples/tlex/*.pla"
)

# Filter out any non-PLA files
list(FILTER PLA_FILES EXCLUDE REGEX "\\.(rom|txt|md|sh)$")

foreach(PLA_FILE ${PLA_FILES})
  get_filename_component(PLA_NAME ${PLA_FILE} NAME_WE)
  get_filename_component(PLA_DIR ${PLA_FILE} DIRECTORY)
  get_filename_component(PLA_DIR_NAME ${PLA_DIR} NAME)
  
  # Create unique test name to avoid conflicts
  set(TEST_NAME "legacy_${PLA_DIR_NAME}_${PLA_NAME}")
  
  add_test(NAME ${TEST_NAME}
    COMMAND sh -c "${CMAKE_CURRENT_BINARY_DIR}/espresso_legacy < ${PLA_FILE}")
  set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 30)
  set_tests_properties(${TEST_NAME} PROPERTIES DEPENDS espresso_legacy)
  set_tests_properties(${TEST_NAME} PROPERTIES LABELS "legacy;regression")
endforeach()

message(STATUS "Legacy espresso CLI built with sanitizers: ${ESPRESSO_LEGACY_SANITIZE}")
