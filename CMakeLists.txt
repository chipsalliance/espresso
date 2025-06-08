cmake_minimum_required(VERSION 3.16)
project(espresso_legacy C)

# Enable strict warnings and sanitizers by default
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Enable AddressSanitizer and UndefinedBehaviorSanitizer by default
option(ESPRESSO_SANITIZE "Enable AddressSanitizer and UBSan" ON)
if(ESPRESSO_SANITIZE)
  set(SAN_FLAGS "-fsanitize=address,undefined" "-fno-omit-frame-pointer")
  add_compile_options(${SAN_FLAGS})
  add_link_options(${SAN_FLAGS})
endif()

# Legacy source files
file(GLOB LEGACY_SRC
  "${CMAKE_SOURCE_DIR}/legacy/*.c"
)

add_executable(espresso_legacy ${LEGACY_SRC})
set_property(TARGET espresso_legacy PROPERTY C_STANDARD 99)

# Install CLI for regression tests
include(GNUInstallDirs)
install(TARGETS espresso_legacy RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Enable CTest and add regression tests for all PLA files
include(CTest)
file(GLOB_RECURSE PLA_FILES
  "${CMAKE_SOURCE_DIR}/examples/*.pla"
  "${CMAKE_SOURCE_DIR}/hard_examples/*.pla"
  "${CMAKE_SOURCE_DIR}/tlex/*.pla"
)
foreach(PLA_FILE ${PLA_FILES})
  get_filename_component(PLA_NAME ${PLA_FILE} NAME_WE)
  add_test(NAME run_${PLA_NAME}
    COMMAND sh -c "${CMAKE_CURRENT_BINARY_DIR}/espresso_legacy < ${PLA_FILE}")
  set_tests_properties(run_${PLA_NAME} PROPERTIES TIMEOUT 30)
  set_tests_properties(run_${PLA_NAME} PROPERTIES DEPENDS espresso_legacy)
endforeach()

message(STATUS "espresso_legacy CLI built with sanitizers: ${ESPRESSO_SANITIZE}")
