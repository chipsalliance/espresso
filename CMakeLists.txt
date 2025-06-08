cmake_minimum_required(VERSION 3.16)
project(espresso CXX C)

# Enable CTest at the top level
include(CTest)

# Add legacy C implementation 
add_subdirectory(legacy)

# Add modern C++23 implementation and tests
add_subdirectory(src)

message(STATUS "Espresso project configured with both legacy C and modern C++ implementations")
