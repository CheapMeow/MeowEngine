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
                      vk::DeviceSize                  size)
            : BufferData(physical_device,
                         device,
                         size,
                         vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
                             vk::BufferUsageFlagBits::eTransferDst,
                         vk::MemoryPropertyFlagBits::eDeviceLocal)
        {}
    };

} // namespace Meow
