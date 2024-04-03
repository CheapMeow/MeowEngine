#include "material.h"

namespace Meow
{
    void Material::CreatePipeline(vk::raii::Device const&     logical_device,
                                  vk::raii::RenderPass const& render_pass,
                                  vk::FrontFace               front_face,
                                  bool                        depth_buffered)
    {
        std::vector<vk::PipelineShaderStageCreateInfo> pipeline_shader_stage_create_infos;
        if (shader_ptr->is_vert_shader_valid)
        {
            pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                            vk::ShaderStageFlagBits::eVertex,
                                                            *shader_ptr->vert_shader_module,
                                                            "main",
                                                            nullptr);
        }
        if (shader_ptr->is_frag_shader_valid)
        {
            pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                            vk::ShaderStageFlagBits::eFragment,
                                                            *shader_ptr->frag_shader_module,
                                                            "main",
                                                            nullptr);
        }
        if (shader_ptr->is_geom_shader_valid)
        {
            pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                            vk::ShaderStageFlagBits::eGeometry,
                                                            *shader_ptr->geom_shader_module,
                                                            "main",
                                                            nullptr);
        }
        if (shader_ptr->is_comp_shader_valid)
        {
            pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                            vk::ShaderStageFlagBits::eCompute,
                                                            *shader_ptr->comp_shader_module,
                                                            "main",
                                                            nullptr);
        }
        if (shader_ptr->is_tesc_shader_valid)
        {
            pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                            vk::ShaderStageFlagBits::eTessellationControl,
                                                            *shader_ptr->tesc_shader_module,
                                                            "main",
                                                            nullptr);
        }
        if (shader_ptr->is_tese_shader_valid)
        {
            pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                            vk::ShaderStageFlagBits::eTessellationEvaluation,
                                                            *shader_ptr->tese_shader_module,
                                                            "main",
                                                            nullptr);
        }

        uint32_t vertex_stride = VertexAttributesToSize(shader_ptr->per_vertex_attributes);

        std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions;
        vk::PipelineVertexInputStateCreateInfo           pipeline_vertex_input_state_create_info;
        vk::VertexInputBindingDescription                vertex_input_binding_description(0, vertex_stride);

        if (0 < vertex_stride)
        {
            uint32_t curr_offset = 0;
            vertex_input_attribute_descriptions.reserve(shader_ptr->per_vertex_attributes.size());
            for (uint32_t i = 0; i < shader_ptr->per_vertex_attributes.size(); i++)
            {
                vertex_input_attribute_descriptions.emplace_back(
                    i, 0, VertexAttributeToVkFormat(shader_ptr->per_vertex_attributes[i]), curr_offset);
                curr_offset += VertexAttributeToSize(shader_ptr->per_vertex_attributes[i]);
            }
            pipeline_vertex_input_state_create_info.setVertexBindingDescriptions(vertex_input_binding_description);
            pipeline_vertex_input_state_create_info.setVertexAttributeDescriptions(vertex_input_attribute_descriptions);
        }

        vk::PipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info(
            vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList);

        vk::PipelineViewportStateCreateInfo pipeline_viewport_state_create_info(
            vk::PipelineViewportStateCreateFlags(), 1, nullptr, 1, nullptr);

        vk::PipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info(
            vk::PipelineRasterizationStateCreateFlags(),
            false,
            false,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            front_face,
            false,
            0.0f,
            0.0f,
            0.0f,
            1.0f);

        vk::PipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info({}, vk::SampleCountFlagBits::e1);

        vk::StencilOpState stencil_op_state(
            vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways);
        vk::PipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info(
            vk::PipelineDepthStencilStateCreateFlags(),
            depth_buffered,
            depth_buffered,
            vk::CompareOp::eLessOrEqual,
            false,
            false,
            stencil_op_state,
            stencil_op_state);

        vk::ColorComponentFlags color_component_flags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
        vk::PipelineColorBlendAttachmentState pipeline_color_blend_attachment_state(false,
                                                                                    vk::BlendFactor::eOne,
                                                                                    vk::BlendFactor::eZero,
                                                                                    vk::BlendOp::eAdd,
                                                                                    vk::BlendFactor::eOne,
                                                                                    vk::BlendFactor::eZero,
                                                                                    vk::BlendOp::eAdd,
                                                                                    color_component_flags);
        vk::PipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info(
            vk::PipelineColorBlendStateCreateFlags(),
            false,
            vk::LogicOp::eNoOp,
            pipeline_color_blend_attachment_state,
            {{1.0f, 1.0f, 1.0f, 1.0f}});

        std::array<vk::DynamicState, 2>    dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info(vk::PipelineDynamicStateCreateFlags(),
                                                                              dynamic_states);

        vk::raii::PipelineCache        pipeline_cache(logical_device, vk::PipelineCacheCreateInfo());
        vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info(vk::PipelineCreateFlags(),
                                                                     pipeline_shader_stage_create_infos,
                                                                     &pipeline_vertex_input_state_create_info,
                                                                     &pipeline_input_assembly_state_create_info,
                                                                     nullptr,
                                                                     &pipeline_viewport_state_create_info,
                                                                     &pipeline_rasterization_state_create_info,
                                                                     &pipeline_multisample_state_create_info,
                                                                     &pipeline_depth_stencil_state_create_info,
                                                                     &pipeline_color_blend_state_create_info,
                                                                     &pipeline_dynamic_state_create_info,
                                                                     *shader_ptr->pipeline_layout,
                                                                     *render_pass);

        graphics_pipeline = vk::raii::Pipeline(logical_device, pipeline_cache, graphics_pipeline_create_info);
    }

    void Material::BindPipeline(vk::raii::CommandBuffer const& command_buffer)
    {
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphics_pipeline);
    }

    void Material::BindDescriptorSets(vk::raii::CommandBuffer const& command_buffer)
    {
        // convert type from vk::raii::DescriptorSet to vk::DescriptorSet

        std::vector<vk::DescriptorSet> descriptor_sets(shader_ptr->descriptor_sets.size());
        for (size_t i = 0; i < descriptor_sets.size(); ++i)
        {
            descriptor_sets[i] = *shader_ptr->descriptor_sets[i];
        }

        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *shader_ptr->pipeline_layout, 0, descriptor_sets, nullptr);
    }
} // namespace Meow
