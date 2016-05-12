cmake_minimum_required(VERSION 3.5)
project(p7)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(Boost COMPONENTS system filesystem thread REQUIRED)

set(SOURCE_FILES main.cpp Server.h Server.cpp ClientShell.cpp ClientShell.h CacheTable.cpp CacheTable.h Data.cpp Data.h)
add_executable(p7 ${SOURCE_FILES})

target_link_libraries(p7 ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY})