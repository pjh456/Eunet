cmake_minimum_required(VERSION 3.10)

project(eunet LANGUAGES CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(eunet STATIC)
add_subdirectory(src)

# libcap-devel checker
find_library(LIBCAP cap)

if(NOT LIBCAP)
    message(FATAL_ERROR "libcap not found, please install libcap-devel")
endif()

target_link_libraries(eunet PRIVATE cap)

add_executable(main src/main.cpp)
target_link_libraries(main eunet)

enable_testing()
add_subdirectory(tests)