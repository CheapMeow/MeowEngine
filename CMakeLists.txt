cmake_minimum_required(VERSION 3.24)

project(MeowEngine LANGUAGES CXX)

# Stop in source builds
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Add the cmake folder so the FindXXX module is found
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

set(ENGINE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_ROOT_DIR ${ENGINE_ROOT_DIR}/src)
set(CODE_GENERATOR_ROOT_DIR ${SRC_ROOT_DIR}/code_generator)
set(3RD_PARTY_ROOT_DIR ${SRC_ROOT_DIR}/3rdparty)
set(RUNTIME_DIR ${SRC_ROOT_DIR}/meow_runtime)
set(EDITOR_DIR ${SRC_ROOT_DIR}/meow_editor)
set(GAME_DIR ${SRC_ROOT_DIR}/meow_game)

set(CODE_GENERATOR_NAME CodeGenerator)
set(GENERATED_FILE_TARGET_NAME GenerateRegisterFile)
set(RUNTIME_NAME MeowRuntime)
set(EDITOR_NAME MeowEditor)
set(GAME_NAME MeowGame)

include(cmake/Utils.cmake)

add_subdirectory(${3RD_PARTY_ROOT_DIR})
add_subdirectory(${CODE_GENERATOR_ROOT_DIR})
add_subdirectory(${RUNTIME_DIR})
add_subdirectory(${EDITOR_DIR})
add_subdirectory(${GAME_DIR})

# Setup editor to be startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT
                                                            ${EDITOR_NAME})

# Set all 3rd party project to one folder
get_all_targets(ALL_TAR_LIST)
foreach(TAR ${ALL_TAR_LIST})
  if("${TAR}" STREQUAL "${CODE_GENERATOR_NAME}"
     OR "${TAR}" STREQUAL "${RUNTIME_NAME}"
     OR "${TAR}" STREQUAL "${EDITOR_NAME}"
     OR "${TAR}" STREQUAL "${GAME_NAME}")
    continue()
  endif()

  # alias target can not call set_target_properties interface library doesn't
  # have folder property so exclude them
  get_property(
    ALIAS
    TARGET "${TAR}"
    PROPERTY ALIASED_TARGET)
  if(NOT ("${ALIAS}" STREQUAL ""))
    continue()
  endif()
  get_target_property(3RD_PARTY_LIB_TYPE ${TAR} TYPE)
  if("${3RD_PARTY_LIB_TYPE}" STREQUAL "INTERFACE_LIBRARY")
    continue()
  endif()

  message(STATUS "Found target: ${TAR}")
  get_property(
    OLD_FOLDER
    TARGET "${TAR}"
    PROPERTY FOLDER)
  set_target_properties(${TAR} PROPERTIES FOLDER "3rdparty/${OLD_FOLDER}")
endforeach()
