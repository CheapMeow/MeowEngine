set(RUNTIME_HEADER_FILES
    pch.h
    runtime.h
    core/base/alignment.h
    core/base/bitmask.hpp
    core/base/log.hpp
    core/base/macro.h
    core/base/non_copyable.h
    core/math/assimp_glm_helper.h
    core/math/bounding_box.h
    core/math/frustum.h
    core/math/math.h
    core/math/plane.h
    core/reflect/macros.h
    core/reflect/reflect.hpp
    core/reflect/reflect_pointer.hpp
    core/reflect/type_descriptor_builder.hpp
    core/reflect/unsafe_any.hpp
    core/signal/signal.hpp
    core/time/scope_time_data.h
    core/time/timer.h
    core/uuid/uuid.h
    function/system.h
    function/components/camera/camera_3d_component.hpp
    function/components/model/model_component.h
    function/components/shared/pawn_component.h
    function/components/shared/uuid_component.h
    function/components/transform/transform_3d_component.hpp
    function/file/file_system.h
    function/global/runtime_context.h
    function/input/input_axis.h
    function/input/input_button.h
    function/input/input_enum.h
    function/input/input_scheme.h
    function/input/input_system.h
    function/input/axes/mouse_input_axis.h
    function/input/buttons/keyboard_input_button.h
    function/input/buttons/mouse_input_button.h
    function/js/js_engine.h
    function/level/level.h
    function/level/level_system.h
    function/object/game_object.h
    function/render/render_system.h
    function/render/render_pass/deferred_pass.h
    function/render/render_pass/forward_pass.h
    function/render/render_pass/render_pass.h
    function/render/render_resources/buffer_data.h
    function/render/structs/descriptor_allocator_growable.h
    function/render/render_resources/image_data.h
    function/render/render_resources/index_buffer.h
    function/render/structs/material.h
    function/render/render_resources/model.hpp
    function/render/render_resources/model_anim.h
    function/render/render_resources/model_bone.h
    function/render/render_resources/model_mesh.h
    function/render/render_resources/model_node.h
    function/render/structs/per_frame_data.h
    function/render/structs/per_scene_data.h
    function/render/structs/pipeline_info.h
    function/render/render_resources/uniform_buffer.h
    function/render/structs/shader.h
    function/render/structs/surface_data.h
    function/render/structs/swapchain_data.h
    function/render/render_resources/vertex_attribute.h
    function/render/render_resources/vertex_buffer.h
    function/render/utils/model_utils.h
    function/render/utils/vulkan_initialize_utils.hpp
    function/render/utils/vulkan_shader_utils.hpp
    function/resource/resource_system.hpp
    function/time/time_system.h
    function/window/window.h
    function/window/window_system.h
    generated/register_all.h
    generated/vertex_attribute_bit.gen.h
    test/geometries.hpp)
set(RUNTIME_SOURCE_FILES
    runtime.cpp
    core/math/frustum.cpp
    core/math/plane.cpp
    core/uuid/uuid.cpp
    function/components/camera/camera_3d_component.cpp
    function/file/file_system.cpp
    function/global/runtime_context.cpp
    function/input/input_system.cpp
    function/input/axes/mouse_input_axis.cpp
    function/input/buttons/keyboard_input_button.cpp
    function/input/buttons/mouse_input_button.cpp
    function/js/js_engine.cpp
    function/level/level.cpp
    function/level/level_system.cpp
    function/object/game_object.cpp
    function/render/render_system.cpp
    function/render/render_pass/deferred_pass.cpp
    function/render/render_pass/forward_pass.cpp
    function/render/render_pass/render_pass.cpp
    function/render/structs/descriptor_allocator_growable.cpp
    function/render/render_resources/image_data.cpp
    function/render/structs/material.cpp
    function/render/render_resources/model.cpp
    function/render/render_resources/model_mesh.cpp
    function/render/render_resources/model_node.cpp
    function/render/render_resources/uniform_buffer.cpp
    function/render/structs/shader.cpp
    function/render/structs/surface_data.cpp
    function/render/structs/swapchain_data.cpp
    function/render/render_resources/vertex_attribute.cpp
    function/render/utils/model_utils.cpp
    function/render/utils/vulkan_initialize_utils.cpp
    function/time/time_system.cpp
    function/window/window.cpp
    function/window/window_system.cpp
    generated/register_all.cpp)

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
    $<$<OR:$<CONFIG:EDITORDEBUG>,$<CONFIG:GAMEDEBUG>>:VKB_DEBUG>
    $<$<OR:$<CONFIG:EDITORDEBUG>,$<CONFIG:GAMEDEBUG>>:VKB_VALIDATION_LAYERS>
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
