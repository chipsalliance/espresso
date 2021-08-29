project(espresso CXX)
cmake_minimum_required(VERSION 3.19)

add_executable(
  espresso
  src/main.cpp
        src/Cover.cpp src/Cover.hpp src/Cube.cpp src/Cube.hpp src/Pla.cpp src/Pla.hpp)

set_property(TARGET espresso PROPERTY CXX_STANDARD 17)
