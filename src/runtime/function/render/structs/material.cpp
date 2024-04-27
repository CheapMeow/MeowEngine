#include "material.h"
#include "core/log/log.h"

#include <algorithm>

namespace Meow
{
    Material::Material(vk::raii::PhysicalDevice const& physical_device,
                       vk::raii::Device const&         logical_device,
                       std::shared_ptr<Shader>         shader_ptr)
        : ring_buffer(physical_device, logical_device)
    {
        this->shader_ptr = shader_ptr;

        // convert type from vk::raii::DescriptorSet to vk::DescriptorSet

        descriptor_sets.resize(shader_ptr->descriptor_sets.size());
        for (size_t i = 0; i < descriptor_sets.size(); ++i)
        {
            descriptor_sets[i] = *shader_ptr->descriptor_sets[i];
        }

        // bind ring buffer to descriptor set
        // There are two parts about binding resources to descriptor set
        // One is binding ring buffer. It is known by Material class, so it is done in Material ctor
        // The second is other's binding, such as image. It is done outside of Material ctor

        for (auto it = shader_ptr->buffer_meta_map.begin(); it != shader_ptr->buffer_meta_map.end(); ++it)
        {
            if (it->second.descriptorType == vk::DescriptorType::eUniformBuffer ||
                it->second.descriptorType == vk::DescriptorType::eUniformBufferDynamic)
            {
                shader_ptr->SetBuffer(
                    logical_device, it->first, ring_buffer.buffer_data_ptr->buffer, it->second.bufferSize);
            }
        }
    }

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
        std::vector<vk::PipelineColorBlendAttachmentState> pipeline_color_blend_attachment_states;
        for (int i = 0; i < color_attachment_count; ++i)
        {
            pipeline_color_blend_attachment_states.emplace_back(false,
                                                                vk::BlendFactor::eOne,
                                                                vk::BlendFactor::eZero,
                                                                vk::BlendOp::eAdd,
                                                                vk::BlendFactor::eOne,
                                                                vk::BlendFactor::eZero,
                                                                vk::BlendOp::eAdd,
                                                                color_component_flags);
        }
        vk::PipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info(
            vk::PipelineColorBlendStateCreateFlags(),
            false,
            vk::LogicOp::eNoOp,
            pipeline_color_blend_attachment_states,
            {{1.0f, 1.0f, 1.0f, 1.0f}});

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

    void Material::BeginFrame()
    {
        if (actived)
        {
            return;
        }
        actived = true;

        // clear per obj data

        obj_count = 0;
        per_obj_dynamic_offsets.clear();

        // copy global uniform buffer data to ring buffer

        // global uniform buffer should be set before BeginFrame() is called
        // so copy global uniform buffer to ring buffer here
        // then it does not need to copy global uniform buffer later during this frame

        for (auto& global_uniform_buffer_info : global_uniform_buffer_infos)
        {
            uint8_t* ringCPUData = (uint8_t*)(ring_buffer.mapped_data_ptr);
            uint64_t bufferSize  = global_uniform_buffer_info.data.size();
            uint64_t ringOffset  = ring_buffer.AllocateMemory(bufferSize);

            memcpy(ringCPUData + ringOffset, global_uniform_buffer_info.data.data(), bufferSize);

            global_uniform_buffer_info.dynamic_offset = (uint32_t)ringOffset;
        }
    }

    void Material::EndFrame()
    {
        actived = false;

        // if no object
        // all elements of per_obj_dynamic_offsets[0] are global uniform buffer offset

        if (per_obj_dynamic_offsets.size() == 0)
        {
            per_obj_dynamic_offsets.push_back(
                std::vector<uint32_t>(shader_ptr->uniform_buffer_count, std::numeric_limits<uint32_t>::max()));

            // copy global uniform buffer offset

            for (auto& global_uniform_buffer_info : global_uniform_buffer_infos)
            {
                per_obj_dynamic_offsets[0][global_uniform_buffer_info.dynamic_offset_index] =
                    global_uniform_buffer_info.dynamic_offset;
            }
        }
    }

    void Material::BeginObject()
    {
        per_obj_dynamic_offsets.push_back(
            std::vector<uint32_t>(shader_ptr->uniform_buffer_count, std::numeric_limits<uint32_t>::max()));

        // copy global uniform buffer offset

        for (auto& global_uniform_buffer_info : global_uniform_buffer_infos)
        {
            per_obj_dynamic_offsets[obj_count][global_uniform_buffer_info.dynamic_offset_index] =
                global_uniform_buffer_info.dynamic_offset;
        }
    }

    void Material::EndObject()
    {
        // check if all dynamic offset is set

        // if there are objects
        if (per_obj_dynamic_offsets.size() == obj_count + 1)
        {
            // check current object
            for (auto offset : per_obj_dynamic_offsets[obj_count])
            {
                if (offset == std::numeric_limits<uint32_t>::max())
                {
                    RUNTIME_ERROR("Uniform not set\n");
                }
            }
        }

        ++obj_count;
    }

    void Material::SetGlobalUniformBuffer(const std::string& name, void* dataPtr, uint32_t size)
    {
        auto buffer_meta_iter = shader_ptr->buffer_meta_map.find(name);
        if (buffer_meta_iter == shader_ptr->buffer_meta_map.end())
        {
            // TODO: format log
            // MLOGE("Uniform %s not found.", name.c_str());
            return;
        }

        if (buffer_meta_iter->second.bufferSize != size)
        {
            // TODO: format log
            // MLOGE("Uniform %s size not match, dst=%ud src=%ud", name.c_str(), it->second.dataSize, size);
            return;
        }

        // store data into info class instance

        auto global_uniform_buffer_info_iter = std::find_if(
            global_uniform_buffer_infos.begin(), global_uniform_buffer_infos.end(), [&](auto& rhs) -> bool {
                return rhs.dynamic_offset_index == buffer_meta_iter->second.dynamic_offset_index;
            });

        if (global_uniform_buffer_info_iter == global_uniform_buffer_infos.end())
        {
            GlobalUniformBufferInfo global_uniform_buffer_info;
            global_uniform_buffer_info.dynamic_offset_index = buffer_meta_iter->second.dynamic_offset_index;
            memcpy(global_uniform_buffer_info.data.data(), dataPtr, size);

            global_uniform_buffer_infos.push_back(global_uniform_buffer_info);
        }
        else
        {
            memcpy(global_uniform_buffer_info_iter->data.data(), dataPtr, size);
        }
    }

    void Material::SetLocalUniformBuffer(const std::string& name, void* dataPtr, uint32_t size)
    {
        auto buffer_meta_iter = shader_ptr->buffer_meta_map.find(name);
        if (buffer_meta_iter == shader_ptr->buffer_meta_map.end())
        {
            // TODO: format log
            // MLOGE("Uniform %s not found.", name.c_str());
            return;
        }

        if (buffer_meta_iter->second.bufferSize != size)
        {
            // TODO: format log
            // MLOGE("Uniform %s size not match, dst=%ud src=%ud", name.c_str(), it->second.dataSize, size);
            return;
        }

        // copy local uniform buffer to ring buffer

        uint8_t* ringCPUData = (uint8_t*)(ring_buffer.mapped_data_ptr);
        uint64_t bufferSize  = buffer_meta_iter->second.bufferSize;
        uint64_t ringOffset  = ring_buffer.AllocateMemory(bufferSize);

        memcpy(ringCPUData + ringOffset, dataPtr, bufferSize);

        per_obj_dynamic_offsets[obj_count][buffer_meta_iter->second.dynamic_offset_index] = (uint32_t)ringOffset;
    }

    void Material::SetStorageBuffer(vk::raii::Device const&     logical_device,
                                    const std::string&          name,
                                    vk::raii::Buffer const&     buffer,
                                    vk::DeviceSize              range,
                                    vk::raii::BufferView const* raii_buffer_view)
    {
        shader_ptr->SetBuffer(logical_device, name, buffer, range, raii_buffer_view);
    }

    void Material::SetImage(vk::raii::Device const& logical_device, const std::string& name, TextureData& texture_data)
    {
        shader_ptr->SetImage(logical_device, name, texture_data);
    }

    void Material::BindPipeline(vk::raii::CommandBuffer const& command_buffer)
    {
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphics_pipeline);
    }

    void Material::BindDescriptorSets(vk::raii::CommandBuffer const& command_buffer, int32_t obj_index)
    {
        if (obj_index >= per_obj_dynamic_offsets.size())
        {
            return;
        }

        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                          *shader_ptr->pipeline_layout,
                                          0,
                                          descriptor_sets,
                                          per_obj_dynamic_offsets[obj_index]);
    }
} // namespace Meow
