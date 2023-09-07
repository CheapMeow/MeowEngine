file(GLOB_RECURSE RUNTIME_HEADER_FILES CONFIGURE_DEPENDS ${SRC_ROOT_DIR}/runtime/*.h)
file(GLOB_RECURSE RUNTIME_SOURCE_FILES CONFIGURE_DEPENDS ${SRC_ROOT_DIR}/runtime/*.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${RUNTIME_HEADER_FILES} ${RUNTIME_SOURCE_FILES})

add_library(${RUNTIME_NAME} SHARED ${RUNTIME_HEADER_FILES} ${RUNTIME_SOURCE_FILES})

set_target_properties(${RUNTIME_NAME} PROPERTIES CXX_STANDARD 17)
set_target_properties(${RUNTIME_NAME} PROPERTIES FOLDER "Engine")

target_compile_definitions(${RUNTIME_NAME}
    PUBLIC
    LIBRARY_EXPORTS)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${RUNTIME_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_compile_options(${RUNTIME_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

target_include_directories(${RUNTIME_NAME} PUBLIC ${SRC_ROOT_DIR}/runtime)

target_link_libraries(${RUNTIME_NAME} PUBLIC spdlog::spdlog)
target_include_directories(${RUNTIME_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/spdlog/include)

target_include_directories(${RUNTIME_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/rocket)

target_link_libraries(${RUNTIME_NAME} PUBLIC glm)
target_include_directories(${RUNTIME_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/glm)

target_link_libraries(${RUNTIME_NAME} PUBLIC glfw)
target_include_directories(${RUNTIME_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/glfw/include)
