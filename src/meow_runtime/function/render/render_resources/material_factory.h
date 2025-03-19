#pragma once

#include "function/render/render_resources/shading_model_type.h"

#include <vulkan/vulkan_raii.hpp>

#include <vector>

namespace Meow
{
    struct PipelineCreateInfoContext
    {
        std::vector<vk::PipelineShaderStageCreateInfo> pipeline_shader_stage_create_infos;
        vk::PipelineVertexInputStateCreateInfo         pipeline_vertex_input_state_create_info;
        vk::PipelineInputAssemblyStateCreateInfo       pipeline_input_assembly_state_create_info;
        vk::PipelineViewportStateCreateInfo            pipeline_viewport_state_create_info;
        vk::PipelineRasterizationStateCreateInfo       pipeline_rasterization_state_create_info;
        vk::PipelineMultisampleStateCreateInfo         pipeline_multisample_state_create_info;
        vk::PipelineDepthStencilStateCreateInfo        pipeline_depth_stencil_state_create_info;
        vk::PipelineColorBlendStateCreateInfo          pipeline_color_blend_state_create_info;
        vk::PipelineDynamicStateCreateInfo             pipeline_dynamic_state_create_info;
    };

    class MaterialFactory
    {};
} // namespace Meow
