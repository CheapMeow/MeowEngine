#pragma once

#include "buffer_data.h"

#include <memory>

namespace Meow
{
    struct StorageBuffer : public BufferData
    {
        VkDeviceSize offset = 0;

        StorageBuffer() {};

        StorageBuffer(vk::raii::PhysicalDevice const& physical_device,
                      vk::raii::Device const&         device,
                      vk::raii::CommandPool const&    command_pool,
                      vk::raii::Queue const&          queue,
                      std::vector<float>&             data)
            : BufferData(physical_device,
                         device,
                         data.size() * sizeof(float),
                         vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
                             vk::BufferUsageFlagBits::eTransferDst,
                         vk::MemoryPropertyFlagBits::eDeviceLocal)
        {
            Upload(physical_device, device, command_pool, queue, data, 0);
        }
    };

} // namespace Meow
