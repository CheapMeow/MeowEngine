#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    /**
     * @brief Info about creating pipeline. These info may be change externel, but most of time the default setting is
     * enough, so the info class is designed.
     *
     * Attachment count is unknown, so convention is max attachment count is 8.
     *
     */
    struct PipelineInfo
    {
        vk::PipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = nullptr;
        vk::PipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info  = nullptr;
        vk::PipelineColorBlendAttachmentState    pipeline_color_blend_attachment_state[8];
        vk::PipelineDepthStencilStateCreateInfo  pipeline_depth_stencil_state_create_info = nullptr;
        vk::PipelineMultisampleStateCreateInfo   pipeline_multisample_state_create_info   = nullptr;
        vk::PipelineTessellationStateCreateInfo  tessellation_state_create_info           = nullptr;

        vk::raii::ShaderModule vert_shader_module = nullptr;
        vk::raii::ShaderModule frag_shader_module = nullptr;
        vk::raii::ShaderModule comp_shader_module = nullptr;
        vk::raii::ShaderModule tesc_shader_module = nullptr;
        vk::raii::ShaderModule tese_shader_module = nullptr;
        vk::raii::ShaderModule geom_shader_module = nullptr;

        PipelineInfo() {}
    };

} // namespace Meow