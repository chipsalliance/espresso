cmake_minimum_required(VERSION 3.16)
project(espresso_cpp_tests CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(espresso_bitcube SHARED
    bitcube.cpp
)

target_include_directories(espresso_bitcube PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(bitcube_tests ../tests/bitcube_tests.cpp)
find_package(Catch2 REQUIRED)
target_link_libraries(bitcube_tests PRIVATE Catch2::Catch2 espresso_bitcube)

enable_testing()
add_test(NAME bitcube_tests COMMAND bitcube_tests)
