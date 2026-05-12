cmake_minimum_required(VERSION 3.29)
project(ALCOR_decoder)

set(CMAKE_CXX_STANDARD 14)

find_package(ROOT REQUIRED COMPONENTS RIO Net)

add_executable(ALCOR_decoder
        check.cpp
        ctest.cpp
        calib.cpp
        cdecoder.cc
        crcheck.cpp)
