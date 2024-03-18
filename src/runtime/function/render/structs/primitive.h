#pragma once

#include "core/base/non_copyable.h"
#include "function/render/structs/buffer_data.h"

namespace Meow
{
    // TODO: Support uint32_t indices
    struct Primitive : NonCopyable
    {
        size_t vertex_count = 0;
        size_t index_count  = 0;

        vk::IndexType index_type = vk::IndexType::eUint16;

        BufferData vertex_buffer = nullptr;
        BufferData index_buffer  = nullptr;

        Primitive() {}

        Primitive(vk::raii::PhysicalDevice const& physical_device,
                  vk::raii::Device const&         device,
                  vk::raii::CommandPool const&    command_pool,
                  vk::raii::Queue const&          queue,
                  vk::MemoryPropertyFlags         property_flags,
                  vk::IndexType                   _index_type,
                  std::vector<float>&             vertices,
                  std::vector<uint16_t>&          indices,
                  size_t                          _vertex_count,
                  size_t                          _index_count
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
                            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                            property_flags)
            , index_buffer(physical_device,
                           device,
                           indices.size() * sizeof(uint16_t),
                           vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                           property_flags)
        {
            vertex_buffer.Upload(physical_device, device, command_pool, queue, vertices, 0);
            index_buffer.Upload(physical_device, device, command_pool, queue, indices, 0);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            std::string                     object_name = std::format("{} {}", primitive_name, "Vertex Buffer");
            vk::DebugUtilsObjectNameInfoEXT name_info   = {
                vk::ObjectType::eBuffer, GetVulkanHandle(*vertex_buffer.buffer), object_name.c_str(), nullptr};
            device.setDebugUtilsObjectNameEXT(name_info);

            object_name = std::format("{} {}", primitive_name, "Vertex Buffer Device Memory");
            name_info   = {vk::ObjectType::eDeviceMemory,
                           GetVulkanHandle(*vertex_buffer.device_memory),
                           object_name.c_str(),
                           nullptr};
            device.setDebugUtilsObjectNameEXT(name_info);

            object_name = std::format("{} {}", primitive_name, "Index Buffer");
            name_info = {vk::ObjectType::eBuffer, GetVulkanHandle(*index_buffer.buffer), object_name.c_str(), nullptr};
            device.setDebugUtilsObjectNameEXT(name_info);

            object_name = std::format("{} {}", primitive_name, "Index Buffer Device Memory");
            name_info   = {vk::ObjectType::eDeviceMemory,
                           GetVulkanHandle(*index_buffer.device_memory),
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