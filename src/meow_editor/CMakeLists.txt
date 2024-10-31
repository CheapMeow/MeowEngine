set(EDITOR_HEADER_FILES
    editor.h
    render/editor_window.h)
set(EDITOR_SOURCE_FILES
    editor.cpp
    main.cpp
    render/editor_window.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_HEADER_FILES} ${EDITOR_SOURCE_FILES})
                                                      
add_executable(${EDITOR_NAME} ${EDITOR_HEADER_FILES} ${EDITOR_SOURCE_FILES})
add_dependencies(${EDITOR_NAME} ${GENERATED_FILE_TARGET_NAME})

set_target_properties(${EDITOR_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${EDITOR_NAME} PROPERTIES FOLDER "Editor")

target_include_directories(${EDITOR_NAME} PUBLIC ${SRC_ROOT_DIR} ${EDITOR_DIR})

target_link_libraries(${EDITOR_NAME} PUBLIC ${RUNTIME_NAME})
target_include_directories(${EDITOR_NAME} PUBLIC ${RUNTIME_DIR})
