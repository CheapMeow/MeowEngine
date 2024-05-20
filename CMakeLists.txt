cmake_minimum_required(VERSION 3.24)
project(MeowEngine CXX)

# Stop in source builds
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Add the cmake folder so the FindXXX module is found
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(ENGINE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_ROOT_DIR ${ENGINE_ROOT_DIR}/src)
set(3RD_PARTY_ROOT_DIR ${SRC_ROOT_DIR}/3rdparty)

set(CODE_GENERATOR_NAME CodeGenerator)
set(GENERATED_FILE_TARGET_NAME generate_register_all)
set(RUNTIME_NAME MeowRuntime)
set(EDITOR_NAME MeowEditor)

include(cmake/Utils.cmake)

add_subdirectory(src/code_generator)
add_custom_command(
  OUTPUT ${SRC_ROOT_DIR}/runtime/generated/register_all.cpp
  COMMAND ${CODE_GENERATOR_NAME} ${SRC_ROOT_DIR}/runtime ${SRC_ROOT_DIR}/runtime/generated
  DEPENDS ${CODE_GENERATOR_NAME}
  COMMENT "Generating register_all.cpp"
)
add_custom_target(${GENERATED_FILE_TARGET_NAME}
    DEPENDS ${SRC_ROOT_DIR}/runtime/generated/register_all.cpp
)

add_subdirectory(src/3rdparty)
add_subdirectory(src/runtime)
add_subdirectory(src/editor)

# Setup editor to be startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT
                                                            ${EDITOR_NAME})

# Docs only available if this is the main app
find_package(Doxygen)
if(Doxygen_FOUND)
  message(STATUS "Found Doxygen, building docs")
  add_subdirectory(docs)
else()
  message(STATUS "Doxygen not found, not building docs")
endif()

# Set all 3rd party project to one folder
get_all_targets(ALL_TAR_LIST)
foreach(TAR ${ALL_TAR_LIST})
  if("${TAR}" STREQUAL "${CODE_GENERATOR_NAME}" OR "${TAR}" STREQUAL "${RUNTIME_NAME}" OR "${TAR}" STREQUAL "${EDITOR_NAME}")
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
