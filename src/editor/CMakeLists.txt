file(GLOB_RECURSE EDITOR_HEADER_FILES CONFIGURE_DEPENDS ${SRC_ROOT_DIR}/editor/*.h ${SRC_ROOT_DIR}/editor/*.hpp)
file(GLOB_RECURSE EDITOR_SOURCE_FILES CONFIGURE_DEPENDS ${SRC_ROOT_DIR}/editor/*.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_HEADER_FILES} ${EDITOR_SOURCE_FILES})

add_executable(${EDITOR_NAME} ${EDITOR_HEADER_FILES} ${EDITOR_SOURCE_FILES})

set_target_properties(${EDITOR_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${EDITOR_NAME} PROPERTIES FOLDER "Editor")

target_compile_definitions(${EDITOR_NAME}
    PRIVATE
    $<$<CONFIG:Debug>:MEOW_DEBUG>
    $<$<CONFIG:Release>:MEOW_RELEASE>
    VULKAN_HPP_STORAGE_SHARED # using dll compiled from vulkan hpp. Only one preprocessor is enough for representing importing.
    )

target_include_directories(${EDITOR_NAME} PUBLIC ${SRC_ROOT_DIR} ${SRC_ROOT_DIR}/editor)

target_link_libraries(${EDITOR_NAME} PUBLIC ${RUNTIME_NAME})
target_include_directories(${EDITOR_NAME} PUBLIC ${SRC_ROOT_DIR}/runtime)

add_custom_command(
    TARGET ${EDITOR_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${EDITOR_NAME}> $<TARGET_FILE_DIR:${EDITOR_NAME}> COMMAND_EXPAND_LISTS)