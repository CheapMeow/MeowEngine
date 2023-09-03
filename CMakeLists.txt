cmake_minimum_required(VERSION 3.24)
project(MeowEngine CXX)

# Stop in source builds
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(ENGINE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SRC_ROOT_DIR "${ENGINE_ROOT_DIR}/src")

# Download dependencies through CMake
include(cmake/dependencies.cmake)

add_subdirectory(src)