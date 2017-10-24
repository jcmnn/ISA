cmake_minimum_required(VERSION 3.1)

project(ISA)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)

find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(lib/capstone)
add_subdirectory(lib/zydis)

add_subdirectory(src)
