cmake_minimum_required(VERSION 3.10)

project(eunet LANGUAGES CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# libcap-devel checker
find_library(LIBCAP cap)

if(NOT LIBCAP)
    message(FATAL_ERROR "libcap not found, please install libcap-devel")
endif()

# fmt-devel checker
find_package(fmt REQUIRED)

# ftxui checker
add_subdirectory(include/FTXUI)

add_library(eunet STATIC)
add_subdirectory(src)

target_link_libraries(eunet PUBLIC ftxui::screen ftxui::dom ftxui::component)
target_link_libraries(eunet PRIVATE cap)
target_link_libraries(eunet PRIVATE fmt::fmt)

add_executable(main src/main.cpp)
target_link_libraries(main eunet)

enable_testing()
add_subdirectory(tests)