set(GAME_HEADER_FILES
    game.h
    render/game_window.h)
set(GAME_SOURCE_FILES
    game.cpp
    main.cpp
    render/game_window.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${GAME_HEADER_FILES} ${GAME_SOURCE_FILES})
                                                      
add_executable(${GAME_NAME} ${GAME_HEADER_FILES} ${GAME_SOURCE_FILES})
add_dependencies(${GAME_NAME} ${GENERATED_FILE_TARGET_NAME})

set_target_properties(${GAME_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${GAME_NAME} PROPERTIES FOLDER "Editor")

target_compile_definitions(
  ${GAME_NAME}
  PRIVATE $<$<CONFIG:Debug>:MEOW_DEBUG>
          $<$<CONFIG:Release>:MEOW_RELEASE>
          VULKAN_HPP_STORAGE_SHARED # using dll compiled from vulkan hpp. Only
                                    # one preprocessor is enough for
                                    # representing importing.
          GLM_ENABLE_EXPERIMENTAL # for GLM: GLM_GTX_component_wise
)

target_include_directories(${GAME_NAME} PUBLIC ${SRC_ROOT_DIR} ${GAME_DIR})

target_link_libraries(${GAME_NAME} PUBLIC ${RUNTIME_NAME})
target_include_directories(${GAME_NAME} PUBLIC ${RUNTIME_DIR})

add_custom_command(
  TARGET ${GAME_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${GAME_NAME}>
          $<TARGET_FILE_DIR:${GAME_NAME}>
  COMMAND_EXPAND_LISTS)
