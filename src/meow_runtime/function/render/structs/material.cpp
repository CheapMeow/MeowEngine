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
            vertex_input_attribute_descriptions.reserve(shader_ptr->per_vertex_attributes.count());
            auto attributes = shader_ptr->per_vertex_attributes.split();
            for (uint32_t i = 0; i < attributes.size(); i++)
            {
                vertex_input_attribute_descriptions.emplace_back(
                    i, 0, VertexAttributeToVkFormat(attributes[i]), curr_offset);
                curr_offset += VertexAttributeToSize(attributes[i]);
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

        vk::StencilOpState stencil_op_state(vk::StencilOp::eKeep,    /* failOp */
                                            vk::StencilOp::eKeep,    /* passOp */
                                            vk::StencilOp::eKeep,    /* depthFailOp */
                                            vk::CompareOp::eAlways); /* compareOp */

        vk::PipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info(
            vk::PipelineDepthStencilStateCreateFlags(), /* flags */
            depth_buffered,                             /* depthTestEnable */
            depth_buffered,                             /* depthWriteEnable */
            vk::CompareOp::eLessOrEqual,                /* depthCompareOp */
            false,                                      /* depthBoundsTestEnable */
            depth_buffered,                             /* stencilTestEnable */
            stencil_op_state,                           /* front */
            stencil_op_state);                          /* back */

        vk::ColorComponentFlags color_component_flags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
        std::vector<vk::PipelineColorBlendAttachmentState> pipeline_color_blend_attachment_states;
        for (int i = 0; i < color_attachment_count; ++i)
        {
            pipeline_color_blend_attachment_states.emplace_back(false,                  /* blendEnable */
                                                                vk::BlendFactor::eOne,  /* srcColorBlendFactor */
                                                                vk::BlendFactor::eZero, /* dstColorBlendFactor */
                                                                vk::BlendOp::eAdd,      /* colorBlendOp */
                                                                vk::BlendFactor::eOne,  /* srcAlphaBlendFactor */
                                                                vk::BlendFactor::eZero, /* dstAlphaBlendFactor */
                                                                vk::BlendOp::eAdd,      /* alphaBlendOp */
                                                                color_component_flags); /* colorWriteMask */
        }
        vk::PipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info(
            vk::PipelineColorBlendStateCreateFlags(), /* flags */
            false,                                    /* logicOpEnable */
            vk::LogicOp::eNoOp,                       /* logicOp */
            pipeline_color_blend_attachment_states,   /* pAttachments */
            {{1.0f, 1.0f, 1.0f, 1.0f}});              /* blendConstants */

        std::array<vk::DynamicState, 2>    dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info(vk::PipelineDynamicStateCreateFlags(),
                                                                              dynamic_states);

        vk::raii::PipelineCache        pipeline_cache(logical_device, vk::PipelineCacheCreateInfo());
        vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info(
            vk::PipelineCreateFlags(),                  /* flags */
            pipeline_shader_stage_create_infos,         /* pStages */
            &pipeline_vertex_input_state_create_info,   /* pVertexInputState */
            &pipeline_input_assembly_state_create_info, /* pInputAssemblyState */
            nullptr,                                    /* pTessellationState */
            &pipeline_viewport_state_create_info,       /* pViewportState */
            &pipeline_rasterization_state_create_info,  /* pRasterizationState */
            &pipeline_multisample_state_create_info,    /* pMultisampleState */
            &pipeline_depth_stencil_state_create_info,  /* pDepthStencilState */
            &pipeline_color_blend_state_create_info,    /* pColorBlendState */
            &pipeline_dynamic_state_create_info,        /* pDynamicState */
            *shader_ptr->pipeline_layout,               /* layout */
            *render_pass,                               /* renderPass */
            subpass);                                   /* subpass */

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
