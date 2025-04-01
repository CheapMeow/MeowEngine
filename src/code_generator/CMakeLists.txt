set(CODE_GENERATOR_HEADER_FILES
    parser/parser.h utils/code_gen_utils.h parse_result/class_parse_result.h
    parse_result/field_parse_result.h parse_result/method_parse_result.h
    generator/code_generator.h)
set(CODE_GENERATOR_SOURCE_FILES
    main.cpp parser/parser.cpp utils/code_gen_utils.cpp
    generator/code_generator.cpp)

file(GLOB_RECURSE RUNTIME_HEADER_FILES "${RUNTIME_DIR}/*.h"
     "${RUNTIME_DIR}/*.hpp")
file(GLOB_RECURSE RUNTIME_SOURCE_FILES "${RUNTIME_DIR}/*.cpp")
file(GLOB_RECURSE EDITOR_HEADER_FILES "${EDITOR_DIR}/*.h" "${EDITOR_DIR}/*.hpp")
file(GLOB_RECURSE EDITOR_SOURCE_FILES "${EDITOR_DIR}/*.cpp")
set(FILES_DEPEND ${RUNTIME_HEADER_FILES} ${RUNTIME_SOURCE_FILES}
                 ${EDITOR_HEADER_FILES} ${EDITOR_SOURCE_FILES})

source_group(
  TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${CODE_GENERATOR_HEADER_FILES}
                                           ${CODE_GENERATOR_SOURCE_FILES})

add_executable(${CODE_GENERATOR_NAME} ${CODE_GENERATOR_HEADER_FILES}
                                      ${CODE_GENERATOR_SOURCE_FILES})

set_target_properties(${CODE_GENERATOR_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${CODE_GENERATOR_NAME} PROPERTIES FOLDER "Engine")

target_include_directories(${CODE_GENERATOR_NAME}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CODE_GENERATOR_NAME}
                      PUBLIC $ENV{LLVM_DIR}/lib/libclang.lib)
target_include_directories(${CODE_GENERATOR_NAME} PUBLIC $ENV{LLVM_DIR}/include)

set(INCLUDE_DIRS)
list(APPEND INCLUDE_DIRS "-I${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND INCLUDE_DIRS "-I${RUNTIME_DIR}")
list(APPEND INCLUDE_DIRS "-I${3RD_PARTY_ROOT_DIR}/glm")
list(APPEND INCLUDE_DIRS "-I${3RD_PARTY_ROOT_DIR}/glfw/include")
list(APPEND INCLUDE_DIRS "-I${3RD_PARTY_ROOT_DIR}/volk")
find_package(Vulkan REQUIRED) # export vars
list(APPEND INCLUDE_DIRS "-I${Vulkan_INCLUDE_DIRS}")
list(APPEND INCLUDE_DIRS "-I${3RD_PARTY_ROOT_DIR}/SPIRV-Cross")
list(APPEND INCLUDE_DIRS "-I${3RD_PARTY_ROOT_DIR}/stb")
list(APPEND INCLUDE_DIRS "-I${3RD_PARTY_ROOT_DIR}/assimp/include")
list(APPEND INCLUDE_DIRS "-I${CMAKE_BINARY_DIR}/src/3rdparty/assimp/include"
)# for config.h
list(APPEND INCLUDE_DIRS "-I${3RD_PARTY_ROOT_DIR}/imgui")
list(APPEND INCLUDE_DIRS "-I$ENV{LLVM_DIR}/include")
list(APPEND INCLUDE_DIRS "-I$ENV{LLVM_DIR}/lib/clang/19/include") # for
                                                                  # mm_malloc.h
                                                                  # to solve
                                                                  # libclang
                                                                  # diagnostic
                                                                  # error

add_custom_command(
  OUTPUT ${RUNTIME_DIR}/generated/register_all.cpp
  COMMAND ${CODE_GENERATOR_NAME} ${INCLUDE_DIRS} "-S${RUNTIME_DIR}"
          "-O${RUNTIME_DIR}/generated"
  DEPENDS ${FILES_DEPEND}
  COMMENT "Generating register_all.cpp")
add_custom_target(${GENERATED_FILE_TARGET_NAME}
                  DEPENDS ${RUNTIME_DIR}/generated/register_all.cpp)
