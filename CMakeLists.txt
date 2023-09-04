cmake_minimum_required(VERSION 3.24)
project(MeowEngine CXX)

# Stop in source builds
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Add the cmake folder so the FindXXX module is found
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(ENGINE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SRC_ROOT_DIR "${ENGINE_ROOT_DIR}/src")

set(RUNTIME_NAME MeowRuntime)
set(EDITOR_NAME MeowEditor)

# Download dependencies through CMake
include(cmake/dependencies.cmake)

add_subdirectory(src)

# Setup editor to be startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${EDITOR_NAME})

# Docs only available if this is the main app
find_package(Doxygen)
if(Doxygen_FOUND)
    message(STATUS "Found Doxygen, building docs")
    add_subdirectory(docs)
else()
    message(STATUS "Doxygen not found, not building docs")
endif()
