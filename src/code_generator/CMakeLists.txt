set(CODE_GENERATOR_HEADER_FILES
    src/parser/parser.h
    src/utils/code_gen_utils.h
    src/parse_result/class_parse_result.h
    src/parse_result/field_parse_result.h
    src/parse_result/method_parse_result.h
    src/generator/code_generator.h)
set(CODE_GENERATOR_SOURCE_FILES
    src/main.cpp
    src/parser/parser.cpp
    src/utils/code_gen_utils.cpp
    src/generator/code_generator.cpp)

set(HEADER_FILES_DEPEND
    ../runtime/engine.h
    ../runtime/pch.h
    ../runtime/core/base/alignment.h
    ../runtime/core/base/bitmask.hpp
    ../runtime/core/base/log.hpp
    ../runtime/core/base/macro.h
    ../runtime/core/base/non_copyable.h
    ../runtime/core/math/assimp_glm_helper.h
    ../runtime/core/math/bounding_box.h
    ../runtime/core/math/frustum.h
    ../runtime/core/math/math.h
    ../runtime/core/math/plane.h
    ../runtime/core/reflect/macros.h
    ../runtime/core/reflect/reflect.hpp
    ../runtime/core/reflect/reflect_pointer.hpp
    ../runtime/core/reflect/type_descriptor_builder.hpp
    ../runtime/core/reflect/unsafe_any.hpp
    ../runtime/core/signal/signal.hpp
    ../runtime/core/time/scope_time_data.h
    ../runtime/core/time/time.h
    ../runtime/core/time/timer.h
    ../runtime/core/uuid/endianness.h
    ../runtime/core/uuid/uuid_v4.h
    ../runtime/function/system.h
    ../runtime/function/components/camera/camera_3d_component.hpp
    ../runtime/function/components/model/model_component.h
    ../runtime/function/components/shared/pawn_component.h
    ../runtime/function/components/shared/uuid_component.h
    ../runtime/function/components/transform/transform_3d_component.hpp
    ../runtime/function/file/file_system.h
    ../runtime/function/global/runtime_global_context.h
    ../runtime/function/input/input_axis.h
    ../runtime/function/input/input_button.h
    ../runtime/function/input/input_enum.h
    ../runtime/function/input/input_scheme.h
    ../runtime/function/input/input_system.h
    ../runtime/function/input/axes/mouse_input_axis.h
    ../runtime/function/input/buttons/keyboard_input_button.h
    ../runtime/function/input/buttons/mouse_input_button.h
    ../runtime/function/level/level.h
    ../runtime/function/level/level_system.h
    ../runtime/function/object/game_object.h
    ../runtime/function/render/render_system.h
    ../runtime/function/render/imgui_widgets/builtin_statistics_widget.h
    ../runtime/function/render/imgui_widgets/components_widget.h
    ../runtime/function/render/imgui_widgets/flame_graph_widget.h
    ../runtime/function/render/imgui_widgets/game_objects_widget.h
    ../runtime/function/render/imgui_widgets/pipeline_statistics_widget.h
    ../runtime/function/render/imgui_widgets/spinner_widget.h
    ../runtime/function/render/render_pass/deferred_pass.h
    ../runtime/function/render/render_pass/forward_pass.h
    ../runtime/function/render/render_pass/imgui_pass.h
    ../runtime/function/render/render_pass/render_pass.h
    ../runtime/function/render/structs/buffer_data.h
    ../runtime/function/render/structs/builtin_render_stat.h
    ../runtime/function/render/structs/descriptor_allocator_growable.h
    ../runtime/function/render/structs/image_data.h
    ../runtime/function/render/structs/index_buffer.h
    ../runtime/function/render/structs/material.h
    ../runtime/function/render/structs/model.h
    ../runtime/function/render/structs/model_anim.h
    ../runtime/function/render/structs/model_bone.h
    ../runtime/function/render/structs/model_mesh.h
    ../runtime/function/render/structs/model_node.h
    ../runtime/function/render/structs/pipeline_info.h
    ../runtime/function/render/structs/ring_uniform_buffer.h
    ../runtime/function/render/structs/ring_uniform_buffer_stat.h
    ../runtime/function/render/structs/shader.h
    ../runtime/function/render/structs/surface_data.h
    ../runtime/function/render/structs/swapchain_data.h
    ../runtime/function/render/structs/ubo_data.h
    ../runtime/function/render/structs/vertex_attribute.h
    ../runtime/function/render/structs/vertex_buffer.h
    ../runtime/function/render/utils/vulkan_initialize_utils.hpp
    ../runtime/function/render/utils/vulkan_shader_utils.hpp
    ../runtime/function/resource/resource_system.h
    ../runtime/function/resource/resource_info/model_res_info.h
    ../runtime/function/window/window.h
    ../runtime/function/window/window_system.h
    ../runtime/generated/register_all.h
    ../runtime/generated/vertex_attribute_bit.gen.h
    ../runtime/test/geometries.hpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${CODE_GENERATOR_HEADER_FILES}
                                                      ${CODE_GENERATOR_SOURCE_FILES})

add_executable(${CODE_GENERATOR_NAME} ${CODE_GENERATOR_HEADER_FILES} ${CODE_GENERATOR_SOURCE_FILES})

find_package(Vulkan REQUIRED) # export vars

target_include_directories(${CODE_GENERATOR_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(${CODE_GENERATOR_NAME} PUBLIC ${SRC_ROOT_DIR}/runtime)
target_include_directories(${CODE_GENERATOR_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/glm)
target_include_directories(${CODE_GENERATOR_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/glfw/include)
target_include_directories(${CODE_GENERATOR_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/volk)
target_include_directories(${CODE_GENERATOR_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_include_directories(${CODE_GENERATOR_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/SPIRV-Cross)
target_include_directories(${CODE_GENERATOR_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/stb)
target_include_directories(${CODE_GENERATOR_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/assimp/include)
target_include_directories(${CODE_GENERATOR_NAME} PUBLIC ${CMAKE_BINARY_DIR}/src/3rdparty/assimp/include) # for config.h
target_include_directories(${CODE_GENERATOR_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/imgui)

function(get_target_include_directories TARGET VAR_NAME)  
    set(INCLUDE_DIRS "")  
    get_target_property(TMP_DIRS ${TARGET} INCLUDE_DIRECTORIES)    
    foreach(DIR ${TMP_DIRS})  
        # If DIR is a generator expression, there will be no expansion here
        # Here we assume they are direct paths 
        list(APPEND INCLUDE_DIRS "-I${DIR}")  
    endforeach()   
    set(${VAR_NAME} "${INCLUDE_DIRS}" PARENT_SCOPE)  
endfunction()  

get_target_include_directories(${CODE_GENERATOR_NAME} INCLUDE_PATH_COLLECTION)  

set_target_properties(${CODE_GENERATOR_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${CODE_GENERATOR_NAME} PROPERTIES FOLDER "Engine")

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${CODE_GENERATOR_NAME}
                       PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_compile_options(${CODE_GENERATOR_NAME}
                       PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

target_link_libraries(${CODE_GENERATOR_NAME} PUBLIC $ENV{LLVM_DIR}/lib/libclang.lib)
target_include_directories(${CODE_GENERATOR_NAME}
                           PUBLIC $ENV{LLVM_DIR}/include)

add_custom_command(
    OUTPUT ${SRC_ROOT_DIR}/runtime/generated/register_all.cpp
    COMMAND ${CODE_GENERATOR_NAME} ${INCLUDE_PATH_COLLECTION} "-S${SRC_ROOT_DIR}/runtime" "-O${SRC_ROOT_DIR}/runtime/generated"
    DEPENDS ${HEADER_FILES_DEPEND}
    COMMENT "Generating register_all.cpp"
)
add_custom_target(${GENERATED_FILE_TARGET_NAME}
    DEPENDS ${SRC_ROOT_DIR}/runtime/generated/register_all.cpp
)