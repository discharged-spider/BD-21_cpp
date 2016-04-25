cmake_minimum_required(VERSION 3.2.2)
project(p6)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(Boost COMPONENTS system filesystem REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

set(SOURCE_FILES main.cpp SharedHashTable.cpp SharedHashTable.h Worker.cpp Worker.h Server.cpp Server.h NetworkTools.h)
add_executable(p6 ${SOURCE_FILES})

target_link_libraries (p6 pthread)