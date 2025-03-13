set(EDITOR_HEADER_FILES
    editor.h
    render/editor_window.h
    render/imgui_widgets/builtin_statistics_widget.h
    render/imgui_widgets/components_widget.h
    render/imgui_widgets/flame_graph_widget.h
    render/imgui_widgets/game_objects_widget.h
    render/imgui_widgets/pipeline_statistics_widget.h
    render/imgui_widgets/spinner_widget.h
    render/render_pass/editor_deferred_pass.h
    render/render_pass/editor_forward_pass.h
    render/render_pass/imgui_pass.h
    render/structs/builtin_render_stat.h
    profile/profile_system.h
    global/editor_context.h)
set(EDITOR_SOURCE_FILES
    editor.cpp
    main.cpp
    render/editor_window.cpp
    render/imgui_widgets/builtin_statistics_widget.cpp
    render/imgui_widgets/components_widget.cpp
    render/imgui_widgets/flame_graph_widget.cpp
    render/imgui_widgets/game_objects_widget.cpp
    render/imgui_widgets/pipeline_statistics_widget.cpp
    render/imgui_widgets/spinner_widget.cpp
    render/render_pass/editor_deferred_pass.cpp
    render/render_pass/editor_forward_pass.cpp
    render/render_pass/imgui_pass.cpp
    profile/profile_system.cpp
    global/editor_context.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_HEADER_FILES}
                                                      ${EDITOR_SOURCE_FILES})

add_executable(${EDITOR_NAME} ${EDITOR_HEADER_FILES} ${EDITOR_SOURCE_FILES})
add_dependencies(${EDITOR_NAME} ${GENERATED_FILE_TARGET_NAME})

target_compile_definitions(${EDITOR_NAME} PUBLIC -DMEOW_EDITOR)

set_target_properties(${EDITOR_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${EDITOR_NAME} PROPERTIES FOLDER "Editor")

target_include_directories(${EDITOR_NAME} PUBLIC ${SRC_ROOT_DIR} ${EDITOR_DIR})

target_link_libraries(${EDITOR_NAME} PUBLIC ${RUNTIME_NAME})
