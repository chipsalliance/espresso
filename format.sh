#!/bin/bash

clang-format -i espresso/*.{h,c}
cmake-format -i CMakeLists.txt
shfmt -i 4 -w format.sh
prettier --write .github/workflows/*
