#include "material_factory.h"

#include "function/global/runtime_context.h"
#include "function/render/utils/vulkan_debug_utils.h"

namespace Meow
{
    void MaterialFactory::Init(const Shader* shader, vk::FrontFace front_face)
    {
        FUNCTION_TIMER();

        if (!shader)
        {
            MEOW_ERROR("shader is nullptr!");
            return;
        }

        std::vector<vk::PipelineShaderStageCreateInfo>().swap(context.pipeline_shader_stage_create_infos);
        if (shader->is_vert_shader_valid)
        {
            context.pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                                    vk::ShaderStageFlagBits::eVertex,
                                                                    *shader->vert_shader_module,
                                                                    "main",
                                                                    nullptr);
        }
        if (shader->is_frag_shader_valid)
        {
            context.pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                                    vk::ShaderStageFlagBits::eFragment,
                                                                    *shader->frag_shader_module,
                                                                    "main",
                                                                    nullptr);
        }
        if (shader->is_geom_shader_valid)
        {
            context.pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                                    vk::ShaderStageFlagBits::eGeometry,
                                                                    *shader->geom_shader_module,
                                                                    "main",
                                                                    nullptr);
        }
        if (shader->is_comp_shader_valid)
        {
            context.pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                                    vk::ShaderStageFlagBits::eCompute,
                                                                    *shader->comp_shader_module,
                                                                    "main",
                                                                    nullptr);
        }
        if (shader->is_tesc_shader_valid)
        {
            context.pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                                    vk::ShaderStageFlagBits::eTessellationControl,
                                                                    *shader->tesc_shader_module,
                                                                    "main",
                                                                    nullptr);
        }
        if (shader->is_tese_shader_valid)
        {
            context.pipeline_shader_stage_create_infos.emplace_back(vk::PipelineShaderStageCreateFlags {},
                                                                    vk::ShaderStageFlagBits::eTessellationEvaluation,
                                                                    *shader->tese_shader_module,
                                                                    "main",
                                                                    nullptr);
        }

        uint32_t vertex_stride = VertexAttributesToSize(shader->per_vertex_attributes);

        std::vector<vk::VertexInputAttributeDescription>().swap(context.vertex_input_attribute_descriptions);
        context.vertex_input_binding_description = vk::VertexInputBindingDescription(0, vertex_stride);

        if (0 < vertex_stride)
        {
            uint32_t curr_offset = 0;
            context.vertex_input_attribute_descriptions.reserve(shader->per_vertex_attributes.size());
            for (uint32_t i = 0; i < shader->per_vertex_attributes.size(); i++)
            {
                context.vertex_input_attribute_descriptions.emplace_back(
                    i, 0, VertexAttributeToVkFormat(shader->per_vertex_attributes[i]), curr_offset);
                curr_offset += VertexAttributeToSize(shader->per_vertex_attributes[i]);
            }
            context.pipeline_vertex_input_state_create_info.setVertexBindingDescriptions(
                context.vertex_input_binding_description);
            context.pipeline_vertex_input_state_create_info.setVertexAttributeDescriptions(
                context.vertex_input_attribute_descriptions);
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

        m_msaa_enabled = false;
        context.pipeline_multisample_state_create_info =
            vk::PipelineMultisampleStateCreateInfo({}, vk::SampleCountFlagBits::e1);

        context.dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        context.pipeline_dynamic_state_create_info =
            vk::PipelineDynamicStateCreateInfo(vk::PipelineDynamicStateCreateFlags(), context.dynamic_states);
    }

    void MaterialFactory::SetMSAA(bool enabled)
    {
        if (m_msaa_enabled == enabled)
        {
            return;
        }

        m_msaa_enabled = enabled;

        if (m_msaa_enabled)
        {
            vk::SampleCountFlagBits sample_count           = g_runtime_context.render_system->GetMSAASamples();
            context.pipeline_multisample_state_create_info = vk::PipelineMultisampleStateCreateInfo({}, sample_count);
        }
        else
        {
            context.pipeline_multisample_state_create_info =
                vk::PipelineMultisampleStateCreateInfo({}, vk::SampleCountFlagBits::e1);
        }
    }

    void MaterialFactory::SetOpaque(bool depth_buffered, int color_attachment_count)
    {
        m_shading_model_type = ShadingModelType::Opaque;

        vk::StencilOpState stencil_op_state(vk::StencilOp::eKeep,    /* failOp */
                                            vk::StencilOp::eKeep,    /* passOp */
                                            vk::StencilOp::eKeep,    /* depthFailOp */
                                            vk::CompareOp::eAlways); /* compareOp */

        context.pipeline_depth_stencil_state_create_info =
            vk::PipelineDepthStencilStateCreateInfo(vk::PipelineDepthStencilStateCreateFlags(), /* flags */
                                                    depth_buffered,                             /* depthTestEnable */
                                                    true,                                       /* depthWriteEnable */
                                                    vk::CompareOp::eLessOrEqual,                /* depthCompareOp */
                                                    false,             /* depthBoundsTestEnable */
                                                    depth_buffered,    /* stencilTestEnable */
                                                    stencil_op_state,  /* front */
                                                    stencil_op_state); /* back */

        vk::ColorComponentFlags color_component_flags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
        std::vector<vk::PipelineColorBlendAttachmentState>().swap(context.pipeline_color_blend_attachment_states);
        for (int i = 0; i < color_attachment_count; ++i)
        {
            context.pipeline_color_blend_attachment_states.emplace_back(
                false,                  /* blendEnable */
                vk::BlendFactor::eOne,  /* srcColorBlendFactor */
                vk::BlendFactor::eZero, /* dstColorBlendFactor */
                vk::BlendOp::eAdd,      /* colorBlendOp */
                vk::BlendFactor::eOne,  /* srcAlphaBlendFactor */
                vk::BlendFactor::eZero, /* dstAlphaBlendFactor */
                vk::BlendOp::eAdd,      /* alphaBlendOp */
                color_component_flags); /* colorWriteMask */
        }
        context.pipeline_color_blend_state_create_info =
            vk::PipelineColorBlendStateCreateInfo(vk::PipelineColorBlendStateCreateFlags(),       /* flags */
                                                  false,                                          /* logicOpEnable */
                                                  vk::LogicOp::eNoOp,                             /* logicOp */
                                                  context.pipeline_color_blend_attachment_states, /* pAttachments */
                                                  {{1.0f, 1.0f, 1.0f, 1.0f}});                    /* blendConstants */
    }

    void MaterialFactory::SetTranslucent(bool depth_buffered, int color_attachment_count)
    {
        m_shading_model_type = ShadingModelType::Translucent;

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
        std::vector<vk::PipelineColorBlendAttachmentState>().swap(context.pipeline_color_blend_attachment_states);
        for (int i = 0; i < color_attachment_count; ++i)
        {
            context.pipeline_color_blend_attachment_states.emplace_back(
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
            vk::PipelineColorBlendStateCreateInfo(vk::PipelineColorBlendStateCreateFlags(),       /* flags */
                                                  false,                                          /* logicOpEnable */
                                                  vk::LogicOp::eNoOp,                             /* logicOp */
                                                  context.pipeline_color_blend_attachment_states, /* pAttachments */
                                                  {{1.0f, 1.0f, 1.0f, 1.0f}});
    }

    void MaterialFactory::CreatePipeline(const vk::raii::Device&     logical_device,
                                         const vk::raii::RenderPass& render_pass,
                                         const class Shader*         shader,
                                         class Material*             material_ptr,
                                         int                         subpass) const
    {
        if (!shader)
        {
            MEOW_ERROR("shader is nullptr!");
            return;
        }

        if (!material_ptr)
        {
            MEOW_ERROR("material_ptr is nullptr!");
            return;
        }

        material_ptr->m_shading_model_type = m_shading_model_type;

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
            *shader->pipeline_layout,                           /* layout */
            *render_pass,                                       /* renderPass */
            subpass);                                           /* subpass */

        material_ptr->graphics_pipeline =
            vk::raii::Pipeline(logical_device, pipeline_cache, graphics_pipeline_create_info);
    }
} // namespace Meow