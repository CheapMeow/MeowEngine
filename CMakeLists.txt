cmake_minimum_required(VERSION 3.24)
project(MeowEngine CXX)

# Stop in source builds
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(ENGINE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SRC_ROOT_DIR "${ENGINE_ROOT_DIR}/src")

set(RUNTIME_NAME MeowRuntime)
set(EDITOR_NAME MeowEditor)

# Download dependencies through CMake
include(cmake/dependencies.cmake)

add_subdirectory(src)

# Setup editor to be startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${EDITOR_NAME})