#include "material_factory.h"

#include "material.h"

#include <array>

namespace Meow
{
    void MaterialFactory::Init(const Shader* shader_ptr, vk::FrontFace front_face)
    {
        FUNCTION_TIMER();

        if (!shader_ptr)
        {
            MEOW_ERROR("shader_ptr is nullptr!");
            return;
        }

        std::vector<vk::PipelineShaderStageCreateInfo>().swap(context.pipeline_shader_stage_create_infos);
        if (shader_ptr->is_vert_shader_valid)
        {
            context.pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                                    vk::ShaderStageFlagBits::eVertex,
                                                                    *shader_ptr->vert_shader_module,
                                                                    "main",
                                                                    nullptr);
        }
        if (shader_ptr->is_frag_shader_valid)
        {
            context.pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                                    vk::ShaderStageFlagBits::eFragment,
                                                                    *shader_ptr->frag_shader_module,
                                                                    "main",
                                                                    nullptr);
        }
        if (shader_ptr->is_geom_shader_valid)
        {
            context.pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                                    vk::ShaderStageFlagBits::eGeometry,
                                                                    *shader_ptr->geom_shader_module,
                                                                    "main",
                                                                    nullptr);
        }
        if (shader_ptr->is_comp_shader_valid)
        {
            context.pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                                    vk::ShaderStageFlagBits::eCompute,
                                                                    *shader_ptr->comp_shader_module,
                                                                    "main",
                                                                    nullptr);
        }
        if (shader_ptr->is_tesc_shader_valid)
        {
            context.pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                                    vk::ShaderStageFlagBits::eTessellationControl,
                                                                    *shader_ptr->tesc_shader_module,
                                                                    "main",
                                                                    nullptr);
        }
        if (shader_ptr->is_tese_shader_valid)
        {
            context.pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                                    vk::ShaderStageFlagBits::eTessellationEvaluation,
                                                                    *shader_ptr->tese_shader_module,
                                                                    "main",
                                                                    nullptr);
        }

        uint32_t vertex_stride = VertexAttributesToSize(shader_ptr->per_vertex_attributes);

        std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions;
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
            context.pipeline_vertex_input_state_create_info.setVertexBindingDescriptions(
                vertex_input_binding_description);
            context.pipeline_vertex_input_state_create_info.setVertexAttributeDescriptions(
                vertex_input_attribute_descriptions);
        }

        context.pipeline_input_assembly_state_create_info = vk::PipelineInputAssemblyStateCreateInfo(
            vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList);

        context.pipeline_viewport_state_create_info =
            vk::PipelineViewportStateCreateInfo(vk::PipelineViewportStateCreateFlags(), 1, nullptr, 1, nullptr);

        context.pipeline_rasterization_state_create_info =
            vk::PipelineRasterizationStateCreateInfo(vk::PipelineRasterizationStateCreateFlags(),
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

        context.pipeline_multisample_state_create_info =
            vk::PipelineMultisampleStateCreateInfo({}, vk::SampleCountFlagBits::e1);

        std::array<vk::DynamicState, 2> dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        context.pipeline_dynamic_state_create_info =
            vk::PipelineDynamicStateCreateInfo(vk::PipelineDynamicStateCreateFlags(), dynamic_states);
    }

    void MaterialFactory::SetOpaque(bool depth_buffered, int color_attachment_count)
    {
        vk::StencilOpState stencil_op_state(vk::StencilOp::eKeep,    /* failOp */
                                            vk::StencilOp::eKeep,    /* passOp */
                                            vk::StencilOp::eKeep,    /* depthFailOp */
                                            vk::CompareOp::eAlways); /* compareOp */

        context.pipeline_depth_stencil_state_create_info =
            vk::PipelineDepthStencilStateCreateInfo(vk::PipelineDepthStencilStateCreateFlags(), /* flags */
                                                    depth_buffered,                             /* depthTestEnable */
                                                    true,                                       /* depthWriteEnable */
                                                    vk::CompareOp::eLessOrEqual,                /* depthCompareOp */
                                                    true,              /* depthBoundsTestEnable */
                                                    depth_buffered,    /* stencilTestEnable */
                                                    stencil_op_state,  /* front */
                                                    stencil_op_state); /* back */

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
        context.pipeline_color_blend_state_create_info =
            vk::PipelineColorBlendStateCreateInfo(vk::PipelineColorBlendStateCreateFlags(), /* flags */
                                                  false,                                    /* logicOpEnable */
                                                  vk::LogicOp::eNoOp,                       /* logicOp */
                                                  pipeline_color_blend_attachment_states,   /* pAttachments */
                                                  {{1.0f, 1.0f, 1.0f, 1.0f}});              /* blendConstants */
    }

    void MaterialFactory::SetTranslucent(bool depth_buffered, int color_attachment_count)
    {
        vk::StencilOpState stencil_op_state(vk::StencilOp::eKeep,    /* failOp */
                                            vk::StencilOp::eKeep,    /* passOp */
                                            vk::StencilOp::eKeep,    /* depthFailOp */
                                            vk::CompareOp::eAlways); /* compareOp */

        context.pipeline_depth_stencil_state_create_info =
            vk::PipelineDepthStencilStateCreateInfo(vk::PipelineDepthStencilStateCreateFlags(), /* flags */
                                                    depth_buffered,                             /* depthTestEnable */
                                                    false,                                      /* depthWriteEnable */
                                                    vk::CompareOp::eLessOrEqual,                /* depthCompareOp */
                                                    false,             /* depthBoundsTestEnable */
                                                    depth_buffered,    /* stencilTestEnable */
                                                    stencil_op_state,  /* front */
                                                    stencil_op_state); /* back */

        vk::ColorComponentFlags color_component_flags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
        std::vector<vk::PipelineColorBlendAttachmentState> pipeline_color_blend_attachment_states;
        for (int i = 0; i < color_attachment_count; ++i)
        {
            pipeline_color_blend_attachment_states.emplace_back(
                true,                               /* blendEnable */
                vk::BlendFactor::eSrcAlpha,         /* srcColorBlendFactor */
                vk::BlendFactor::eOneMinusSrcAlpha, /* dstColorBlendFactor */
                vk::BlendOp::eAdd,                  /* colorBlendOp */
                vk::BlendFactor::eOne,              /* srcAlphaBlendFactor */
                vk::BlendFactor::eZero,             /* dstAlphaBlendFactor */
                vk::BlendOp::eAdd,                  /* alphaBlendOp */
                color_component_flags);             /* colorWriteMask */
        }
        context.pipeline_color_blend_state_create_info =
            vk::PipelineColorBlendStateCreateInfo(vk::PipelineColorBlendStateCreateFlags(), /* flags */
                                                  false,                                    /* logicOpEnable */
                                                  vk::LogicOp::eNoOp,                       /* logicOp */
                                                  pipeline_color_blend_attachment_states,   /* pAttachments */
                                                  {{1.0f, 1.0f, 1.0f, 1.0f}});
    }

    void MaterialFactory::CreatePipeline(const vk::raii::Device&     logical_device,
                                         const vk::raii::RenderPass& render_pass,
                                         const class Shader*         shader_ptr,
                                         class Material*             material_ptr,
                                         int                         subpass) const
    {
        if (!shader_ptr)
        {
            MEOW_ERROR("shader_ptr is nullptr!");
            return;
        }

        if (!material_ptr)
        {
            MEOW_ERROR("material_ptr is nullptr!");
            return;
        }

        vk::raii::PipelineCache        pipeline_cache(logical_device, vk::PipelineCacheCreateInfo());
        vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info(
            vk::PipelineCreateFlags(),                          /* flags */
            context.pipeline_shader_stage_create_infos,         /* pStages */
            &context.pipeline_vertex_input_state_create_info,   /* pVertexInputState */
            &context.pipeline_input_assembly_state_create_info, /* pInputAssemblyState */
            nullptr,                                            /* pTessellationState */
            &context.pipeline_viewport_state_create_info,       /* pViewportState */
            &context.pipeline_rasterization_state_create_info,  /* pRasterizationState */
            &context.pipeline_multisample_state_create_info,    /* pMultisampleState */
            &context.pipeline_depth_stencil_state_create_info,  /* pDepthStencilState */
            &context.pipeline_color_blend_state_create_info,    /* pColorBlendState */
            &context.pipeline_dynamic_state_create_info,        /* pDynamicState */
            *shader_ptr->pipeline_layout,                       /* layout */
            *render_pass,                                       /* renderPass */
            subpass);                                           /* subpass */

        material_ptr->graphics_pipeline =
            vk::raii::Pipeline(logical_device, pipeline_cache, graphics_pipeline_create_info);
    }
} // namespace Meow