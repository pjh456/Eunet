cmake_minimum_required(VERSION 3.16)

project(eunet LANGUAGES CXX)

# 是否构建单元测试
option(BUILD_TESTS "Build unit tests" OFF)

# 强制关闭 FTXUI 的单元测试
set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # libcap-devel checker
find_library(LIBCAP cap)

if(NOT LIBCAP)
    message(FATAL_ERROR "libcap not found, please install libcap-devel")
endif()

# boost checker
find_package(Boost REQUIRED COMPONENTS system)

# fmt-devel checker
find_package(fmt REQUIRED)

# ftxui checker
add_subdirectory(include/FTXUI)

add_library(eunet STATIC)
add_subdirectory(src)

target_link_libraries(eunet PUBLIC ftxui::screen ftxui::dom ftxui::component)
target_link_libraries(eunet PRIVATE cap)
target_link_libraries(eunet PRIVATE Boost::system)
target_link_libraries(eunet PRIVATE fmt::fmt)

add_executable(eunet_cli src/main.cpp)
target_link_libraries(eunet_cli eunet)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()