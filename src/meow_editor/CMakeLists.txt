file(GLOB_RECURSE EDITOR_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
file(GLOB_RECURSE EDITOR_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_HEADER_FILES}
                                                      ${EDITOR_SOURCE_FILES})

add_executable(${EDITOR_NAME} ${EDITOR_HEADER_FILES} ${EDITOR_SOURCE_FILES})
add_dependencies(${EDITOR_NAME} ${GENERATED_FILE_TARGET_NAME})

target_compile_definitions(${EDITOR_NAME} PUBLIC -DMEOW_EDITOR)

set_target_properties(${EDITOR_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${EDITOR_NAME} PROPERTIES FOLDER "Editor")

target_include_directories(${EDITOR_NAME} PUBLIC ${SRC_ROOT_DIR} ${EDITOR_DIR})

target_link_libraries(${EDITOR_NAME} PUBLIC ${RUNTIME_NAME})
target_link_libraries(${EDITOR_NAME} PUBLIC imguizmo)
