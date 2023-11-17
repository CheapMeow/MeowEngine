file(GLOB_RECURSE RUNTIME_HEADER_FILES CONFIGURE_DEPENDS ${SRC_ROOT_DIR}/runtime/*.h ${SRC_ROOT_DIR}/runtime/*.hpp)
file(GLOB_RECURSE RUNTIME_SOURCE_FILES CONFIGURE_DEPENDS ${SRC_ROOT_DIR}/runtime/*.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${RUNTIME_HEADER_FILES} ${RUNTIME_SOURCE_FILES})

add_library(${RUNTIME_NAME} SHARED ${RUNTIME_HEADER_FILES} ${RUNTIME_SOURCE_FILES})

set_target_properties(${RUNTIME_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${RUNTIME_NAME} PROPERTIES FOLDER "Engine")

target_compile_definitions(${RUNTIME_NAME}
    PRIVATE
    LIBRARY_EXPORTS
    $<$<CONFIG:Debug>:MEOW_DEBUG>
    $<$<CONFIG:Release>:MEOW_RELEASE>
    $<$<CONFIG:Debug>:VKB_DEBUG>
    $<$<CONFIG:Debug>:VKB_VALIDATION_LAYERS>
    VK_USE_PLATFORM_WIN32_KHR # TODO: Config vulkan platform preprocesser according to cmake build platform
    VULKAN_HPP_STORAGE_SHARED # compile dll when using vulkan hpp
    VULKAN_HPP_STORAGE_SHARED_EXPORT # export DispatchLoaderDynamic from vulkan hpp
    NOMINMAX # for std::numeric_limits<double>::min() and max()
    )

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
target_compile_definitions(${RUNTIME_NAME}
    PUBLIC
    GLFW_INCLUDE_VULKAN 
    GLFW_EXPOSE_NATIVE_WIN32)

target_link_libraries(${RUNTIME_NAME} PUBLIC volk)
target_include_directories(${RUNTIME_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/volk)
target_compile_definitions(${RUNTIME_NAME} PUBLIC VK_NO_PROTOTYPES)

if(Vulkan_FOUND)
    # for vulkan hpp
    target_include_directories(${RUNTIME_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
    # don't link to vulkan static lib
endif()

target_link_libraries(${RUNTIME_NAME} PUBLIC 
    glslang
    glslang-default-resource-limits
    SPIRV)
target_include_directories(${RUNTIME_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/glslang)

target_link_libraries(${RUNTIME_NAME} PUBLIC stb)
target_include_directories(${RUNTIME_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/stb)

target_link_libraries(${RUNTIME_NAME} PUBLIC assimp)
target_include_directories(${RUNTIME_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/assimp/include)

target_link_libraries(${RUNTIME_NAME} PUBLIC imgui)
target_include_directories(${RUNTIME_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/imgui)

target_link_libraries(${RUNTIME_NAME} PUBLIC EnTT::EnTT)
target_include_directories(${RUNTIME_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/entt/src)