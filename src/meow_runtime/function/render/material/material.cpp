#include "material.h"

#include "pch.h"

#include "function/global/runtime_context.h"
#include "function/render/utils/vulkan_debug_utils.h"

#include <algorithm>

namespace Meow
{
    Material::Material(std::shared_ptr<Shader> shader) { this->shader = shader; }

    void Material::CreateUniformBuffer()
    {
        const auto k_max_frames_in_flight = g_runtime_context.render_system->GetMaxFramesInFlight();

        m_dynamic_uniform_buffer_per_frame.resize(k_max_frames_in_flight);

        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        for (auto it = shader->buffer_meta_map.begin(); it != shader->buffer_meta_map.end(); ++it)
        {
            if (it->second.descriptorType == vk::DescriptorType::eUniformBuffer)
            {
                m_uniform_buffers_per_frame[it->first].resize(k_max_frames_in_flight);
                for (uint32_t i = 0; i < k_max_frames_in_flight; ++i)
                {
                    m_uniform_buffers_per_frame[it->first][i] =
                        std::move(std::make_unique<UniformBuffer>(physical_device, logical_device, it->second.size));
                    BindBufferToDescriptorSet(
                        it->first, m_uniform_buffers_per_frame[it->first][i]->buffer, VK_WHOLE_SIZE, nullptr, i);
                }
            }
            if (it->second.descriptorType == vk::DescriptorType::eUniformBufferDynamic)
            {
                for (uint32_t i = 0; i < k_max_frames_in_flight; ++i)
                {
                    if (!m_dynamic_uniform_buffer_per_frame[i])
                        m_dynamic_uniform_buffer_per_frame[i] =
                            std::move(std::make_unique<UniformBuffer>(physical_device, logical_device, 32 * 1024));
                    BindBufferToDescriptorSet(
                        it->first, m_dynamic_uniform_buffer_per_frame[i]->buffer, VK_WHOLE_SIZE, nullptr, i);
                }
            }
        }
    }

    void Material::BindPipeline(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        command_buffer.bindPipeline(m_bind_point, *m_pipeline);
    }

    void Material::BindBufferToDescriptorSet(const std::string&          name,
                                             const vk::raii::Buffer&     buffer,
                                             vk::DeviceSize              range,
                                             const vk::raii::BufferView* raii_buffer_view,
                                             uint32_t                    frame_index)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        BufferMeta* meta = nullptr;
        // If it is dynamic uniform buffer, then the buffer passed into can not use whole size
        for (auto it = shader->buffer_meta_map.begin(); it != shader->buffer_meta_map.end(); ++it)
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
                if (it->second.descriptorType == vk::DescriptorType::eStorageBuffer)
                {
                    meta = &it->second;
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
            *m_descriptor_sets_per_frame[frame_index][meta->set],                 // dstSet
            meta->binding,                                                        // dstBinding
            0,                                                                    // dstArrayElement
            1,                                                                    // descriptorCount
            shader->set_layout_metas.GetDescriptorType(meta->set, meta->binding), // descriptorType
            nullptr,                                                              // pImageInfo
            &descriptor_buffer_info,                                              // pBufferInfo
            raii_buffer_view ? &buffer_view : nullptr                             // pTexelBufferView
        );

        logical_device.updateDescriptorSets(write_descriptor_set, nullptr);
    }

    void Material::BindImageToDescriptorSet(const std::string& name, ImageData& image_data, uint32_t frame_index)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        auto it = shader->set_layout_metas.binding_meta_map.find(name);
        if (it == shader->set_layout_metas.binding_meta_map.end())
        {
            MEOW_ERROR("Writing buffer failed, {} not found!", name);
            return;
        }

        auto bindInfo = it->second;

        vk::DescriptorImageInfo descriptor_image_info(
            *image_data.sampler, *image_data.image_view, vk::ImageLayout::eShaderReadOnlyOptimal);

        vk::WriteDescriptorSet write_descriptor_set(
            *m_descriptor_sets_per_frame[frame_index][bindInfo.set],                    // dstSet
            bindInfo.binding,                                                           // dstBinding
            0,                                                                          // dstArrayElement
            1,                                                                          // descriptorCount
            shader->set_layout_metas.GetDescriptorType(bindInfo.set, bindInfo.binding), // descriptorType
            &descriptor_image_info,                                                     // pImageInfo
            nullptr,                                                                    // pBufferInfo
            nullptr                                                                     // pTexelBufferView
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
        for (auto& dynamic_uniform_buffer : m_dynamic_uniform_buffer_per_frame)
        {
            dynamic_uniform_buffer->ResetMemory();
        }
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
            std::vector<uint32_t>(shader->dynamic_uniform_buffer_count, std::numeric_limits<uint32_t>::max()));
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

    void
    Material::PopulateDynamicUniformBuffer(const std::string& name, void* data, uint32_t size, uint32_t frame_index)
    {
        FUNCTION_TIMER();

        if (!shader)
        {
            MEOW_ERROR("shader is null");
            return;
        }

        auto it = shader->buffer_meta_map.find(name);
        if (it == shader->buffer_meta_map.end())
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

        m_per_obj_dynamic_offsets[m_obj_count][it->second.dynamic_seq] =
            static_cast<uint32_t>(m_dynamic_uniform_buffer_per_frame[frame_index]->Populate(data, it->second.size));
    }

    void Material::PopulateUniformBuffer(const std::string& name, void* data, uint32_t size, uint32_t frame_index)
    {
        if (!shader)
        {
            MEOW_ERROR("shader is null");
            return;
        }

        auto it = shader->buffer_meta_map.find(name);
        if (it == shader->buffer_meta_map.end())
        {
            MEOW_ERROR("Uniform {} not found.", name);
            return;
        }

        if (it->second.size != size)
        {
            MEOW_WARN("Uniform {} size not match, dst={} src={}", name, it->second.size, size);
        }

        m_uniform_buffers_per_frame[it->first][frame_index]->ResetMemory();
        m_uniform_buffers_per_frame[it->first][frame_index]->Populate(data, size);
    }

    void Material::BindDescriptorSetToPipeline(const vk::raii::CommandBuffer& command_buffer,
                                               uint32_t                       first_set,
                                               uint32_t                       set_count,
                                               uint32_t                       draw_call,
                                               bool                           is_dynamic,
                                               uint32_t                       frame_index)
    {
        if (is_dynamic)
        {
            if (draw_call >= m_per_obj_dynamic_offsets.size())
            {
                MEOW_ERROR("Draw call exceed dynamic offset count!");
                return;
            }
        }

        std::vector<vk::DescriptorSet> descriptor_sets_to_bind(set_count);
        for (uint32_t i = first_set; i < first_set + set_count; ++i)
        {
            descriptor_sets_to_bind[i - first_set] = *m_descriptor_sets_per_frame[frame_index][i];
        }

        std::vector<uint32_t> dynamic_offsets {};
        if (is_dynamic)
            dynamic_offsets = m_per_obj_dynamic_offsets[draw_call];

        command_buffer.bindDescriptorSets(
            m_bind_point, *shader->pipeline_layout, first_set, descriptor_sets_to_bind, dynamic_offsets);
    }

    void Material::SetDebugName(const std::string& debug_name, uint32_t frame_index)
    {
        if (debug_name.empty())
            return;

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        {
            for (size_t i = 0; i < m_descriptor_sets_per_frame[frame_index].size(); i++)
            {
                std::string descriptor_set_name =
                    std::format("{} DescriptorSet {} frame {}", debug_name, i, frame_index);

                vk::DebugUtilsObjectNameInfoEXT name_info = {
                    vk::ObjectType::eDescriptorSet,
                    NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkDescriptorSet,
                                                           *m_descriptor_sets_per_frame[frame_index][i]),
                    descriptor_set_name.c_str()};
                logical_device.setDebugUtilsObjectNameEXT(name_info);
            }
        }

        {
            std::string pipeline_name = std::format("{} Pipeline {}", debug_name, frame_index);

            vk::DebugUtilsObjectNameInfoEXT name_info = {
                vk::ObjectType::ePipeline,
                NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkPipeline, *m_pipeline),
                pipeline_name.c_str()};
            logical_device.setDebugUtilsObjectNameEXT(name_info);
        }
#endif
    }

    void swap(Material& lhs, Material& rhs)
    {
        using std::swap;

        swap(static_cast<ResourceBase&>(lhs), static_cast<ResourceBase&>(rhs));

        std::swap(lhs.shader, rhs.shader);
        std::swap(lhs.m_pipeline, rhs.m_pipeline);
        std::swap(lhs.m_actived, rhs.m_actived);
        std::swap(lhs.m_obj_count, rhs.m_obj_count);
        std::swap(lhs.m_per_obj_dynamic_offsets, rhs.m_per_obj_dynamic_offsets);
        std::swap(lhs.m_descriptor_sets_per_frame, rhs.m_descriptor_sets_per_frame);
        std::swap(lhs.m_uniform_buffers_per_frame, rhs.m_uniform_buffers_per_frame);
        std::swap(lhs.m_dynamic_uniform_buffer_per_frame, rhs.m_dynamic_uniform_buffer_per_frame);
    }
} // namespace Meow
