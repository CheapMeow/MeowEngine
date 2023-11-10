#pragma once

#include "core/base/non_copyable.h"
#include "function/systems/render/utils/vulkan_hpp_utils.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    // TODO: Support uint32_t indices
    struct Primitive : NonCopyable
    {
        size_t vertex_count = 0;
        size_t index_count  = 0;

        vk::IndexType index_type = vk::IndexType::eUint16;

        vk::Meow::BufferData vertex_buffer = nullptr;
        vk::Meow::BufferData index_buffer  = nullptr;

        Primitive() {}

        Primitive(std::vector<float>&             vertices,
                  std::vector<uint16_t>&          indices,
                  size_t                          _vertex_count,
                  size_t                          _index_count,
                  vk::raii::PhysicalDevice const& physical_device,
                  vk::raii::Device const&         device,
                  vk::MemoryPropertyFlags         property_flags = vk::MemoryPropertyFlagBits::eHostVisible |
                                                           vk::MemoryPropertyFlagBits::eHostCoherent,
                  vk::IndexType _index_type = vk::IndexType::eUint16
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
                  ,
                  std::string primitive_name = "Default Primitive Name"
#endif
                  )
            : vertex_count(_vertex_count)
            , index_count(_index_count)
            , index_type(_index_type)
            , vertex_buffer(physical_device,
                            device,
                            vertices.size() * sizeof(float),
                            vk::BufferUsageFlagBits::eVertexBuffer,
                            property_flags)
            , index_buffer(physical_device,
                           device,
                           indices.size() * sizeof(uint16_t),
                           vk::BufferUsageFlagBits::eIndexBuffer,
                           property_flags)
        {

            vk::Meow::CopyToDevice(vertex_buffer.device_memory, vertices.data(), vertices.size());

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            std::string                     object_name = std::format("{} {}", primitive_name, "Vertex Buffer");
            vk::DebugUtilsObjectNameInfoEXT name_info   = {vk::ObjectType::eBuffer,
                                                           vk::Meow::GetVulkanHandle(*vertex_buffer.buffer),
                                                           object_name.c_str(),
                                                           nullptr};
            device.setDebugUtilsObjectNameEXT(name_info);

            object_name = std::format("{} {}", primitive_name, "Vertex Buffer Device Memory");
            name_info   = {vk::ObjectType::eDeviceMemory,
                           vk::Meow::GetVulkanHandle(*vertex_buffer.device_memory),
                           object_name.c_str(),
                           nullptr};
            device.setDebugUtilsObjectNameEXT(name_info);
#endif

            vk::Meow::CopyToDevice(index_buffer.device_memory, indices.data(), indices.size());

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            object_name = std::format("{} {}", primitive_name, "Index Buffer");
            name_info   = {
                vk::ObjectType::eBuffer, vk::Meow::GetVulkanHandle(*index_buffer.buffer), object_name.c_str(), nullptr};
            device.setDebugUtilsObjectNameEXT(name_info);

            object_name = std::format("{} {}", primitive_name, "Index Buffer Device Memory");
            name_info   = {vk::ObjectType::eDeviceMemory,
                           vk::Meow::GetVulkanHandle(*index_buffer.device_memory),
                           object_name.c_str(),
                           nullptr};
            device.setDebugUtilsObjectNameEXT(name_info);
#endif
        }

        void BindDrawCmd(const vk::raii::CommandBuffer& cmd_buffer) const
        {
            cmd_buffer.bindVertexBuffers(0, {*vertex_buffer.buffer}, {0});
            cmd_buffer.bindIndexBuffer(*index_buffer.buffer, 0, index_type);
            cmd_buffer.drawIndexed(index_count, 1, 0, 0, 0);
        }
    };
} // namespace Meow