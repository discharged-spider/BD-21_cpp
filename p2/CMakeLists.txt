cmake_minimum_required(VERSION 3.2)
project(p2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

set(SOURCE_FILES_S chatsrv.cpp Server.cpp Server.h)
add_executable(chatsrv ${SOURCE_FILES_S})