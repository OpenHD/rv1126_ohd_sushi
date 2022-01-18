cmake_minimum_required(VERSION 3.20)
project(consti_demo)

set(CMAKE_CXX_STANDARD 17)

add_executable(consti_demo
        src/v4l2_test_mplane.cpp)
