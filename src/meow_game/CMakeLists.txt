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

target_include_directories(${GAME_NAME} PUBLIC ${SRC_ROOT_DIR} ${GAME_DIR})

target_link_libraries(${GAME_NAME} PUBLIC ${RUNTIME_NAME})
target_include_directories(${GAME_NAME} PUBLIC ${RUNTIME_DIR})
