#include "material.h"

#include "function/global/runtime_global_context.h"
#include "function/systems/render/structs/vertex_attribute.h"

namespace Meow
{
    Material::Material(vk::raii::Device const&              logical_device,
                       vk::raii::DescriptorSetLayout const& descriptor_set_layout,
                       vk::raii::RenderPass const&          render_pass)
    {
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, *descriptor_set_layout);
        pipeline_layout = vk::raii::PipelineLayout(logical_device, pipeline_layout_create_info);

        // TODO: temp Shader
        uint8_t* data_ptr = nullptr;
        uint32_t data_size;
        g_runtime_global_context.file_system.get()->ReadBinaryFile(
            "builtin/shaders/mesh.vert.spv", data_ptr, data_size);
        vk::raii::ShaderModule vertex_shader_module(logical_device,
                                                    {vk::ShaderModuleCreateFlags(), data_size, (uint32_t*)data_ptr});
        delete[] data_ptr;
        data_ptr = nullptr;
        g_runtime_global_context.file_system.get()->ReadBinaryFile(
            "builtin/shaders/mesh.frag.spv", data_ptr, data_size);
        vk::raii::ShaderModule fragment_shader_module(logical_device,
                                                      {vk::ShaderModuleCreateFlags(), data_size, (uint32_t*)data_ptr});
        delete[] data_ptr;
        data_ptr = nullptr;

        // TODO: temp vertex layout
        vk::raii::PipelineCache pipeline_cache(logical_device, vk::PipelineCacheCreateInfo());
        graphics_pipeline = vk::Meow::MakeGraphicsPipeline(
            logical_device,
            pipeline_cache,
            vertex_shader_module,
            nullptr,
            fragment_shader_module,
            nullptr,
            VertexAttributesToSize({VertexAttribute::VA_Position, VertexAttribute::VA_Normal}),
            {{vk::Format::eR32G32B32Sfloat, 0}, {vk::Format::eR32G32B32Sfloat, 12}},
            vk::FrontFace::eClockwise,
            true,
            pipeline_layout,
            render_pass);
    }

    void Material::Bind(vk::raii::CommandBuffer const& command_buffer, vk::raii::DescriptorSet const& descriptor_set)
    {
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphics_pipeline);
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0, {*descriptor_set}, nullptr);
    }
} // namespace Meow