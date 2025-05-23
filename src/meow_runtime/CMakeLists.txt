file(GLOB_RECURSE RUNTIME_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
file(GLOB_RECURSE RUNTIME_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${RUNTIME_HEADER_FILES}
                                                      ${RUNTIME_SOURCE_FILES})

add_library(${RUNTIME_NAME} STATIC ${RUNTIME_HEADER_FILES}
                                   ${RUNTIME_SOURCE_FILES})
add_dependencies(${RUNTIME_NAME} ${GENERATED_FILE_TARGET_NAME})

set_target_properties(${RUNTIME_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${RUNTIME_NAME} PROPERTIES FOLDER "Engine")

target_compile_definitions(${RUNTIME_NAME} PRIVATE LIBRARY_EXPORTS)
target_compile_definitions(
  ${RUNTIME_NAME}
  PUBLIC
    ENGINE_ROOT_DIR="${ENGINE_ROOT_DIR}"
    $<$<CONFIG:Debug>:VKB_DEBUG>
    $<$<CONFIG:Debug>:VKB_VALIDATION_LAYERS>
    VK_USE_PLATFORM_WIN32_KHR # TODO: Config vulkan platform preprocesser
                              # according to cmake build platform
    GLM_ENABLE_EXPERIMENTAL # for GLM: GLM_GTX_component_wise
    IMGUI_DEFINE_MATH_OPERATORS
    # _DISABLE_STRING_ANNOTATION # for ASAN _DISABLE_VECTOR_ANNOTATION # for
    # ASAN
    NOMINMAX # for std::numeric_limits<double>::min() and max()
)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${RUNTIME_NAME}
                       PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_compile_options(${RUNTIME_NAME}
                       PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

# ASAN set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

target_include_directories(${RUNTIME_NAME} PUBLIC ${RUNTIME_DIR})

target_link_libraries(${RUNTIME_NAME} PUBLIC glm)
target_include_directories(${RUNTIME_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/glm)

target_link_libraries(${RUNTIME_NAME} PUBLIC glfw)
target_include_directories(${RUNTIME_NAME}
                           PUBLIC ${3RD_PARTY_ROOT_DIR}/glfw/include)
target_compile_definitions(${RUNTIME_NAME} PUBLIC GLFW_INCLUDE_VULKAN
                                                  GLFW_EXPOSE_NATIVE_WIN32)

target_link_libraries(${RUNTIME_NAME} PUBLIC volk)
target_include_directories(${RUNTIME_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/volk)
target_compile_definitions(${RUNTIME_NAME} PUBLIC VK_NO_PROTOTYPES)

if(Vulkan_FOUND)
  # for vulkan hpp
  target_include_directories(${RUNTIME_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
  # don't link to vulkan static lib
endif()

target_link_libraries(
  ${RUNTIME_NAME}
  PUBLIC spirv-cross-glsl
         spirv-cross-hlsl
         spirv-cross-cpp
         spirv-cross-reflect
         spirv-cross-msl
         spirv-cross-util
         spirv-cross-core)
target_include_directories(${RUNTIME_NAME}
                           PUBLIC ${3RD_PARTY_ROOT_DIR}/SPIRV-Cross)

target_link_libraries(${RUNTIME_NAME} PUBLIC stb)
target_include_directories(${RUNTIME_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/stb)

target_link_libraries(${RUNTIME_NAME} PUBLIC assimp)
target_include_directories(${RUNTIME_NAME}
                           PUBLIC ${3RD_PARTY_ROOT_DIR}/assimp/include)

target_link_libraries(${RUNTIME_NAME} PUBLIC imgui)
target_include_directories(${RUNTIME_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/imgui)

target_link_libraries(${RUNTIME_NAME} PUBLIC qjs)
