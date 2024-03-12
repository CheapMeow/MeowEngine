#pragma once

#include "buffer_data.h"
#include "vertex_attribute.h"

#include <memory>

namespace Meow
{
    struct VertexBuffer
    {
        std::shared_ptr<BufferData> buffer_data_ptr = nullptr;
        VkDeviceSize                offset          = 0;

        VertexBuffer(vk::raii::PhysicalDevice const& physical_device,
                     vk::raii::Device const&         device,
                     vk::raii::CommandPool const&    command_pool,
                     vk::raii::Queue const&          queue,
                     vk::MemoryPropertyFlags         property_flags,
                     std::vector<float>&             vertices)
        {
            buffer_data_ptr = std::make_shared<BufferData>(physical_device,
                                                           device,
                                                           vertices.size() * sizeof(float),
                                                           vk::BufferUsageFlagBits::eVertexBuffer |
                                                               vk::BufferUsageFlagBits::eTransferDst,
                                                           property_flags);
            buffer_data_ptr->Upload(physical_device, device, command_pool, queue, vertices, 0);
        }
    };

} // namespace Meow
