cmake_minimum_required(VERSION 3.29)
project(ALCOR_decoder)

set(CMAKE_CXX_STANDARD 14)

#find_package(ROOT REQUIRED COMPONENTS RIO Net)

add_executable(ALCOR_decoder
        data_structs.h
        data_structs51.h
        parameters.txt
        check.cpp
        ctest.cpp
        calib.cpp
        cdecoder.cc
        crcheck.cpp
        calib51Q.cpp)

