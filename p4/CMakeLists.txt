cmake_minimum_required(VERSION 3.2.2)
project(p4)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

set(SOURCE_FILES main.cpp BufferFileReader.cpp BufferFileReader.h IndexReverser.cpp IndexReverser.h)
add_executable(p4 ${SOURCE_FILES})