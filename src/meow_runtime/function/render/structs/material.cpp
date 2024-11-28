#include "material.h"

#include "pch.h"

#include "function/global/runtime_context.h"

#include <algorithm>

namespace Meow
{
    Material::Material(std::shared_ptr<Shader> shader_ptr)
    {
        DescriptorAllocatorGrowable& descriptor_allocator = g_runtime_context.render_system->GetDescriptorAllocator();

        this->shader_ptr  = shader_ptr;
        m_descriptor_sets = descriptor_allocator.Allocate(shader_ptr->descriptor_set_layouts);

        CreateUniformBuffer();
    }

    void Material::CreateUniformBuffer()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        for (auto it = shader_ptr->buffer_meta_map.begin(); it != shader_ptr->buffer_meta_map.end(); ++it)
        {
            if (it->second.descriptorType == vk::DescriptorType::eUniformBuffer)
            {
                m_uniform_buffers[it->first] =
                    std::move(std::make_unique<UniformBuffer>(physical_device, logical_device, it->second.size));
                BindBufferToDescriptorSet(it->first, m_uniform_buffers[it->first]->buffer);
            }
            if (it->second.descriptorType == vk::DescriptorType::eUniformBufferDynamic)
            {
                if (!m_dynamic_uniform_buffer)
                    m_dynamic_uniform_buffer =
                        std::move(std::make_unique<UniformBuffer>(physical_device, logical_device, 32 * 1024));
                BindBufferToDescriptorSet(it->first, m_dynamic_uniform_buffer->buffer);
            }
        }
    }

    void Material::CreatePipeline2(vk::FrontFace front_face, bool depth_buffered)
    {
        FUNCTION_TIMER();

        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

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
            pipeline_color_blend_attachment_states.emplace_back(false,                  /* blendEnable */
                                                                vk::BlendFactor::eOne,  /* srcColorBlendFactor */
                                                                vk::BlendFactor::eZero, /* dstColorBlendFactor */
                                                                vk::BlendOp::eAdd,      /* colorBlendOp */
                                                                vk::BlendFactor::eOne,  /* srcAlphaBlendFactor */
                                                                vk::BlendFactor::eZero, /* dstAlphaBlendFactor */
                                                                vk::BlendOp::eAdd,      /* alphaBlendOp */
                                                                color_component_flags); /* colorWriteMask */
        vk::PipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info(
            vk::PipelineColorBlendStateCreateFlags(), /* flags */
            false,                                    /* logicOpEnable */
            vk::LogicOp::eNoOp,                       /* logicOp */
            pipeline_color_blend_attachment_states,   /* pAttachments */
            {{1.0f, 1.0f, 1.0f, 1.0f}});              /* blendConstants */

        std::array<vk::DynamicState, 2>    dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info(vk::PipelineDynamicStateCreateFlags(),
                                                                              dynamic_states);

        std::vector<vk::Format> color_attachment_formats;
        for (int i = 0; i < color_attachment_count; ++i)
            color_attachment_formats.push_back(vk::Format::eR8G8B8A8Unorm);
        vk::PipelineRenderingCreateInfoKHR rendering_create_info_KHR(
            {},                       /* viewMask */
            color_attachment_formats, /* colorAttachmentFormats_ */
            vk::Format::eD16Unorm     /* depthAttachmentFormat_ */
        );

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
            nullptr,                                    /* renderPass */
            {},                                         /* subpass */
            {},                                         /* basePipelineHandle */
            {},                                         /* basePipelineIndex */
            &rendering_create_info_KHR);                /* pNext */

        graphics_pipeline = vk::raii::Pipeline(logical_device, pipeline_cache, graphics_pipeline_create_info);
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

    void Material::BindBufferToDescriptorSet(const std::string&          name,
                                             const vk::raii::Buffer&     buffer,
                                             vk::DeviceSize              range,
                                             const vk::raii::BufferView* raii_buffer_view)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        BufferMeta* meta = nullptr;
        // If it is dynamic uniform buffer, then the buffer passed into can not use whole size
        for (auto it = shader_ptr->buffer_meta_map.begin(); it != shader_ptr->buffer_meta_map.end(); ++it)
        {
            if (it->first == name)
            {
                if (it->second.descriptorType == vk::DescriptorType::eUniformBuffer)
                {
                    meta = &it->second;
                    break;
                }
                if (it->second.descriptorType == vk::DescriptorType::eUniformBufferDynamic)
                {
                    meta  = &it->second;
                    range = meta->size;
                    break;
                }
            }
        }

        if (meta == nullptr)
        {
            MEOW_ERROR("Binding buffer failed, {} not found!", name);
            return;
        }

        vk::DescriptorBufferInfo descriptor_buffer_info(*buffer, 0, range);

        // TODO: store buffer view in an vector
        vk::BufferView buffer_view;
        if (raii_buffer_view)
        {
            buffer_view = **raii_buffer_view;
        }

        vk::WriteDescriptorSet write_descriptor_set(
            *m_descriptor_sets[meta->set],                                            // dstSet
            meta->binding,                                                            // dstBinding
            0,                                                                        // dstArrayElement
            1,                                                                        // descriptorCount
            shader_ptr->set_layout_metas.GetDescriptorType(meta->set, meta->binding), // descriptorType
            nullptr,                                                                  // pImageInfo
            &descriptor_buffer_info,                                                  // pBufferInfo
            raii_buffer_view ? &buffer_view : nullptr                                 // pTexelBufferView
        );

        logical_device.updateDescriptorSets(write_descriptor_set, nullptr);
    }

    void Material::BindImageToDescriptorSet(const std::string& name, ImageData& image_data)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        auto it = shader_ptr->set_layout_metas.binding_meta_map.find(name);
        if (it == shader_ptr->set_layout_metas.binding_meta_map.end())
        {
            MEOW_ERROR("Writing buffer failed, {} not found!", name);
            return;
        }

        auto bindInfo = it->second;

        vk::DescriptorImageInfo descriptor_image_info(
            *image_data.sampler, *image_data.image_view, vk::ImageLayout::eShaderReadOnlyOptimal);

        vk::WriteDescriptorSet write_descriptor_set(
            *m_descriptor_sets[bindInfo.set],                                               // dstSet
            bindInfo.binding,                                                               // dstBinding
            0,                                                                              // dstArrayElement
            1,                                                                              // descriptorCount
            shader_ptr->set_layout_metas.GetDescriptorType(bindInfo.set, bindInfo.binding), // descriptorType
            &descriptor_image_info,                                                         // pImageInfo
            nullptr,                                                                        // pBufferInfo
            nullptr                                                                         // pTexelBufferView
        );

        logical_device.updateDescriptorSets(write_descriptor_set, nullptr);
    }

    void Material::BeginPopulatingDynamicUniformBufferPerFrame()
    {
        FUNCTION_TIMER();

        if (m_actived)
        {
            return;
        }
        m_actived = true;

        // clear per obj data

        m_obj_count = 0;
        m_per_obj_dynamic_offsets.clear();
        m_dynamic_uniform_buffer->Reset();
    }

    void Material::EndPopulatingDynamicUniformBufferPerFrame()
    {
        FUNCTION_TIMER();

        m_actived = false;
    }

    void Material::BeginPopulatingDynamicUniformBufferPerObject()
    {
        FUNCTION_TIMER();

        m_per_obj_dynamic_offsets.push_back(
            std::vector<uint32_t>(shader_ptr->dynamic_uniform_buffer_count, std::numeric_limits<uint32_t>::max()));
    }

    void Material::EndPopulatingDynamicUniformBufferPerObject()
    {
        FUNCTION_TIMER();

        // check if all dynamic offset is set

        // if there are objects
        if (m_per_obj_dynamic_offsets.size() == m_obj_count + 1)
        {
            // check current object
            for (auto offset : m_per_obj_dynamic_offsets[m_obj_count])
            {
                if (offset == std::numeric_limits<uint32_t>::max())
                {
                    MEOW_ERROR("Uniform not set");
                }
            }
        }

        ++m_obj_count;
    }

    void Material::PopulateDynamicUniformBuffer(const std::string& name, void* data, uint32_t size)
    {
        FUNCTION_TIMER();

        auto it = shader_ptr->buffer_meta_map.find(name);
#ifdef MEOW_DEBUG
        if (it == shader_ptr->buffer_meta_map.end())
        {
            MEOW_ERROR("Uniform {} not found.", name);
            return;
        }

        if (it->second.size != size)
        {
            MEOW_WARN("Uniform {} size not match, dst={} src={}", name, it->second.size, size);
        }

        if (m_per_obj_dynamic_offsets[m_obj_count].size() <= it->second.dynamic_seq)
        {
            MEOW_ERROR("per_obj_dynamic_offsets[obj_count] size {} <= dynamic sequence {} from uniform buffer meta.",
                       m_per_obj_dynamic_offsets[m_obj_count].size(),
                       it->second.dynamic_seq);
            return;
        }
#endif

        m_per_obj_dynamic_offsets[m_obj_count][it->second.dynamic_seq] =
            static_cast<uint32_t>(m_dynamic_uniform_buffer->Populate(data, it->second.size));
    }

    void Material::PopulateUniformBuffer(const std::string& name, void* data, uint32_t size)
    {

        auto it = shader_ptr->buffer_meta_map.find(name);
#ifdef MEOW_DEBUG
        if (it == shader_ptr->buffer_meta_map.end())
        {
            MEOW_ERROR("Uniform {} not found.", name);
            return;
        }

        if (it->second.size != size)
        {
            MEOW_WARN("Uniform {} size not match, dst={} src={}", name, it->second.size, size);
        }
#endif

        m_uniform_buffers[it->first]->Reset();
        m_uniform_buffers[it->first]->Populate(data, size);
    }

    void Material::BindDescriptorSetToPipeline(const vk::raii::CommandBuffer& command_buffer,
                                               uint32_t                       first_set,
                                               uint32_t                       set_count,
                                               uint32_t                       draw_call,
                                               bool                           is_dynamic)
    {
        if (is_dynamic)
        {
            if (draw_call >= m_per_obj_dynamic_offsets.size())
            {
                return;
            }
        }

        std::vector<vk::DescriptorSet> descriptor_sets_to_bind(set_count);
        for (uint32_t i = first_set; i < first_set + set_count; ++i)
        {
            descriptor_sets_to_bind[i - first_set] = *m_descriptor_sets[i];
        }

        std::vector<uint32_t> dynamic_offsets {};
        if (is_dynamic)
            dynamic_offsets = m_per_obj_dynamic_offsets[draw_call];

        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                          *shader_ptr->pipeline_layout,
                                          first_set,
                                          descriptor_sets_to_bind,
                                          dynamic_offsets);
    }

    void swap(Material& lhs, Material& rhs)
    {
        using std::swap;

        std::swap(lhs.shader_ptr, rhs.shader_ptr);
        std::swap(lhs.color_attachment_count, rhs.color_attachment_count);
        std::swap(lhs.subpass, rhs.subpass);

        std::swap(lhs.graphics_pipeline, rhs.graphics_pipeline);
        std::swap(lhs.m_actived, rhs.m_actived);
        std::swap(lhs.m_obj_count, rhs.m_obj_count);
        std::swap(lhs.m_per_obj_dynamic_offsets, rhs.m_per_obj_dynamic_offsets);
        std::swap(lhs.m_descriptor_sets, rhs.m_descriptor_sets);
        std::swap(lhs.m_uniform_buffers, rhs.m_uniform_buffers);
        std::swap(lhs.m_dynamic_uniform_buffer, rhs.m_dynamic_uniform_buffer);
    }
} // namespace Meow
