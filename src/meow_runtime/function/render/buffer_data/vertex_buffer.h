#pragma once

#include "buffer_data.h"
#include "vertex_attribute.h"

#include <memory>

namespace Meow
{
    struct VertexBuffer : public BufferData
    {
        VkDeviceSize offset = 0;

        VertexBuffer(vk::raii::PhysicalDevice const& physical_device,
                     vk::raii::Device const&         device,
                     vk::raii::CommandPool const&    command_pool,
                     vk::raii::Queue const&          queue,
                     std::vector<float>&             vertices)
            : BufferData(physical_device,
                         device,
                         vertices.size() * sizeof(float),
                         vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                         vk::MemoryPropertyFlagBits::eDeviceLocal)
        {
            Upload(physical_device, device, command_pool, queue, vertices, 0);
        }
    };

} // namespace Meow
