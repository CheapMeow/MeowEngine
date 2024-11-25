#include "material.h"

#include "pch.h"

#include <algorithm>

namespace Meow
{
    Material::Material(const vk::raii::PhysicalDevice& physical_device,
                       const vk::raii::Device&         logical_device,
                       std::shared_ptr<Shader>         shader_ptr)
    {
        this->shader_ptr = shader_ptr;
    }

    void Material::CreatePipeline(const vk::raii::Device&     logical_device,
                                  const vk::raii::RenderPass& render_pass,
                                  vk::FrontFace               front_face,
                                  bool                        depth_buffered)
    {
        FUNCTION_TIMER();

        std::vector<vk::PipelineShaderStageCreateInfo> pipeline_shader_stage_create_infos;
        if (shader_ptr->is_vert_shader_valid)
        {
            pipeline_shader_stage_create_infos.push_back(vk::PipelineShaderStageCreateInfo {
                .stage  = vk::ShaderStageFlagBits::eVertex,
                .module = *shader_ptr->vert_shader_module,
                .pName  = "main",
            });
        }
        if (shader_ptr->is_frag_shader_valid)
        {
            pipeline_shader_stage_create_infos.push_back(vk::PipelineShaderStageCreateInfo {
                .stage  = vk::ShaderStageFlagBits::eFragment,
                .module = *shader_ptr->frag_shader_module,
                .pName  = "main",
            });
        }
        if (shader_ptr->is_geom_shader_valid)
        {
            pipeline_shader_stage_create_infos.push_back(vk::PipelineShaderStageCreateInfo {
                .stage  = vk::ShaderStageFlagBits::eGeometry,
                .module = *shader_ptr->geom_shader_module,
                .pName  = "main",
            });
        }
        if (shader_ptr->is_comp_shader_valid)
        {
            pipeline_shader_stage_create_infos.push_back(vk::PipelineShaderStageCreateInfo {
                .stage  = vk::ShaderStageFlagBits::eCompute,
                .module = *shader_ptr->comp_shader_module,
                .pName  = "main",
            });
        }
        if (shader_ptr->is_tesc_shader_valid)
        {
            pipeline_shader_stage_create_infos.push_back(vk::PipelineShaderStageCreateInfo {
                .stage  = vk::ShaderStageFlagBits::eTessellationControl,
                .module = *shader_ptr->tesc_shader_module,
                .pName  = "main",
            });
        }
        if (shader_ptr->is_tese_shader_valid)
        {
            pipeline_shader_stage_create_infos.push_back(vk::PipelineShaderStageCreateInfo {
                .stage  = vk::ShaderStageFlagBits::eTessellationEvaluation,
                .module = *shader_ptr->tese_shader_module,
                .pName  = "main",
            });
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
                vertex_input_attribute_descriptions.push_back(vk::VertexInputAttributeDescription {
                    .location = i,
                    .binding  = 0,
                    .format   = VertexAttributeToVkFormat(shader_ptr->per_vertex_attributes[i]),
                    .offset   = curr_offset,
                });
                curr_offset += VertexAttributeToSize(shader_ptr->per_vertex_attributes[i]);
            }
            pipeline_vertex_input_state_create_info.setVertexBindingDescriptions(vertex_input_binding_description);
            pipeline_vertex_input_state_create_info.setVertexAttributeDescriptions(vertex_input_attribute_descriptions);
        }

        vk::PipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = {
            .topology = vk::PrimitiveTopology::eTriangleList};

        vk::PipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {.viewportCount = 1,
                                                                                   .scissorCount  = 1};

        vk::PipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = {
            .depthClampEnable        = false,
            .rasterizerDiscardEnable = false,
            .polygonMode             = vk::PolygonMode::eFill,
            .cullMode                = vk::CullModeFlagBits::eBack,
            .frontFace               = front_face,
            .depthBiasEnable         = false,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp          = 0.0f,
            .depthBiasSlopeFactor    = 0.0f,
            .lineWidth               = 1.0f};

        vk::PipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = {
            .rasterizationSamples = vk::SampleCountFlagBits::e1};

        vk::StencilOpState stencil_op_state = {.failOp      = vk::StencilOp::eKeep,
                                               .passOp      = vk::StencilOp::eKeep,
                                               .depthFailOp = vk::StencilOp::eKeep,
                                               .compareOp   = vk::CompareOp::eAlways};

        vk::PipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = {
            .depthTestEnable       = depth_buffered,
            .depthWriteEnable      = depth_buffered,
            .depthCompareOp        = vk::CompareOp::eLessOrEqual,
            .depthBoundsTestEnable = false,
            .stencilTestEnable     = depth_buffered,
            .front                 = stencil_op_state,
            .back                  = stencil_op_state};

        vk::ColorComponentFlags color_component_flags = vk::ColorComponentFlagBits::eR |
                                                        vk::ColorComponentFlagBits::eG |
                                                        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        std::vector<vk::PipelineColorBlendAttachmentState> pipeline_color_blend_attachment_states;
        for (int i = 0; i < color_attachment_count; ++i)
        {
            pipeline_color_blend_attachment_states.push_back(
                vk::PipelineColorBlendAttachmentState {.blendEnable         = false,
                                                       .srcColorBlendFactor = vk::BlendFactor::eOne,
                                                       .dstColorBlendFactor = vk::BlendFactor::eZero,
                                                       .colorBlendOp        = vk::BlendOp::eAdd,
                                                       .srcAlphaBlendFactor = vk::BlendFactor::eOne,
                                                       .dstAlphaBlendFactor = vk::BlendFactor::eZero,
                                                       .alphaBlendOp        = vk::BlendOp::eAdd,
                                                       .colorWriteMask      = color_component_flags});
        }
        vk::PipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = {
            .logicOpEnable   = false,
            .logicOp         = vk::LogicOp::eNoOp,
            .attachmentCount = static_cast<uint32_t>(pipeline_color_blend_attachment_states.size()),
            .pAttachments    = pipeline_color_blend_attachment_states.data(),
            .blendConstants  = {{1.0f, 1.0f, 1.0f, 1.0f}}};

        std::array<vk::DynamicState, 2>    dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info = {
            .dynamicStateCount = dynamic_states.size(), .pDynamicStates = dynamic_states.data()};

        vk::raii::PipelineCache        pipeline_cache(logical_device, vk::PipelineCacheCreateInfo());
        vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info = {
            .stageCount          = static_cast<uint32_t>(pipeline_shader_stage_create_infos.size()),
            .pStages             = pipeline_shader_stage_create_infos.data(),
            .pVertexInputState   = &pipeline_vertex_input_state_create_info,
            .pInputAssemblyState = &pipeline_input_assembly_state_create_info,
            .pTessellationState  = nullptr,
            .pViewportState      = &pipeline_viewport_state_create_info,
            .pRasterizationState = &pipeline_rasterization_state_create_info,
            .pMultisampleState   = &pipeline_multisample_state_create_info,
            .pDepthStencilState  = &pipeline_depth_stencil_state_create_info,
            .pColorBlendState    = &pipeline_color_blend_state_create_info,
            .pDynamicState       = &pipeline_dynamic_state_create_info,
            .layout              = *shader_ptr->pipeline_layout,
            .renderPass          = *render_pass,
            .subpass             = subpass};

        graphics_pipeline = vk::raii::Pipeline(logical_device, pipeline_cache, graphics_pipeline_create_info);
    }

    void Material::BindPipeline(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphics_pipeline);
    }

    void Material::BeginPopulatingDynamicUniformBufferPerFrame()
    {
        FUNCTION_TIMER();

        if (actived)
        {
            return;
        }
        actived = true;

        // clear per obj data

        obj_count = 0;
        per_obj_dynamic_offsets.clear();
    }

    void Material::EndPopulatingDynamicUniformBufferPerFrame()
    {
        FUNCTION_TIMER();

        actived = false;
    }

    void Material::BeginPopulatingDynamicUniformBufferPerObject()
    {
        FUNCTION_TIMER();

        per_obj_dynamic_offsets.push_back(
            std::vector<uint32_t>(shader_ptr->dynamic_uniform_buffer_count, std::numeric_limits<uint32_t>::max()));
    }

    void Material::EndPopulatingDynamicUniformBufferPerObject()
    {
        FUNCTION_TIMER();

        // check if all dynamic offset is set

        // if there are objects
        if (per_obj_dynamic_offsets.size() == obj_count + 1)
        {
            // check current object
            for (auto offset : per_obj_dynamic_offsets[obj_count])
            {
                if (offset == std::numeric_limits<uint32_t>::max())
                {
                    MEOW_ERROR("Uniform not set");
                }
            }
        }

        ++obj_count;
    }

    void Material::PopulateDynamicUniformBuffer(std::shared_ptr<UniformBuffer> buffer,
                                                const std::string&             name,
                                                void*                          dataPtr,
                                                uint32_t                       size)
    {
        FUNCTION_TIMER();

        auto it = shader_ptr->buffer_meta_map.find(name);
        if (it == shader_ptr->buffer_meta_map.end())
        {
            MEOW_ERROR("Uniform {} not found.", name);
            return;
        }

        if (it->second.size != size)
        {
            MEOW_WARN("Uniform {} size not match, dst={} src={}", name, it->second.size, size);
        }

        if (per_obj_dynamic_offsets[obj_count].size() <= it->second.dynamic_seq)
        {
            MEOW_ERROR("per_obj_dynamic_offsets[obj_count] size {} <= dynamic sequence {} from uniform buffer meta.",
                       per_obj_dynamic_offsets[obj_count].size(),
                       it->second.dynamic_seq);
            return;
        }

        per_obj_dynamic_offsets[obj_count][it->second.dynamic_seq] =
            static_cast<uint32_t>(buffer->Populate(dataPtr, it->second.size));
    }

    std::vector<uint32_t> Material::GetDynamicOffsets(uint32_t obj_index)
    {
        FUNCTION_TIMER();

        if (obj_index >= per_obj_dynamic_offsets.size())
        {
            return {};
        }

        return per_obj_dynamic_offsets[obj_index];
    }
} // namespace Meow
