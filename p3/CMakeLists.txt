cmake_minimum_required(VERSION 3.2)
project(p3)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(Boost COMPONENTS system filesystem REQUIRED)

set(SOURCE_FILES main.cpp Server.cpp Server.h Connection.h Pipe.h)
add_executable(p3 ${SOURCE_FILES})

target_link_libraries(p3 ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})
