#pragma once

#include "material.h"
#include "shading_model_type.h"

#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <vector>

namespace Meow
{
    struct PipelineCreateInfoContext
    {
        std::vector<vk::PipelineShaderStageCreateInfo>   pipeline_shader_stage_create_infos;
        std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions;
        vk::VertexInputBindingDescription                vertex_input_binding_description;
        vk::PipelineVertexInputStateCreateInfo           pipeline_vertex_input_state_create_info;
        vk::PipelineInputAssemblyStateCreateInfo         pipeline_input_assembly_state_create_info;
        vk::PipelineViewportStateCreateInfo              pipeline_viewport_state_create_info;
        vk::PipelineRasterizationStateCreateInfo         pipeline_rasterization_state_create_info;
        vk::PipelineMultisampleStateCreateInfo           pipeline_multisample_state_create_info;
        vk::PipelineDepthStencilStateCreateInfo          pipeline_depth_stencil_state_create_info;
        vk::PipelineColorBlendStateCreateInfo            pipeline_color_blend_state_create_info;
        std::array<vk::DynamicState, 2>                  dynamic_states;
        vk::PipelineDynamicStateCreateInfo               pipeline_dynamic_state_create_info;
    };

    class MaterialFactory
    {
    public:
        void Init(const Shader* shader_ptr, vk::FrontFace front_face = vk::FrontFace::eClockwise);
        void SetOpaque(bool depth_buffered = true, int color_attachment_count = 1);
        void SetTranslucent(bool depth_buffered = true, int color_attachment_count = 1);
        void CreatePipeline(const vk::raii::Device&     logical_device,
                            const vk::raii::RenderPass& render_pass,
                            const Shader*               shader_ptr,
                            Material*                   material_ptr,
                            int                         subpass = 0) const;

    private:
        PipelineCreateInfoContext context;
    };
} // namespace Meow
