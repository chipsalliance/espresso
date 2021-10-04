project(espresso CXX C)
cmake_minimum_required(VERSION 3.19)

add_library(smmincov STATIC
        src/SparseMatrix/mincov.c
        src/SparseMatrix/matrix.c
        src/SparseMatrix/cols.c
        src/SparseMatrix/rows.c
        src/SparseMatrix/solution.c
        src/SparseMatrix/dominate.c
        src/SparseMatrix/gimpel.c
        src/SparseMatrix/indep.c
        src/SparseMatrix/part.c
        )

add_executable(
  espresso
  src/main.cpp
        src/Cover.cpp src/Cover.hpp src/Cube.cpp src/Cube.hpp src/Pla.cpp src/Pla.hpp src/SMMincov.cpp src/Cost.cpp src/Cost.hpp)
target_link_libraries(espresso smmincov)

set_property(TARGET espresso PROPERTY CXX_STANDARD 20)
