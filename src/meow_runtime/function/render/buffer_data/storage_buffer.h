#pragma once

#include "buffer_data.h"

#include <memory>

namespace Meow
{
    struct StorageBuffer : BufferData
    {
        VkDeviceSize offset = 0;

        StorageBuffer() {};

        StorageBuffer(const vk::raii::PhysicalDevice& physical_device,
                      const vk::raii::Device&         device,
                      const vk::raii::CommandPool&    command_pool,
                      const vk::raii::Queue&          queue,
                      vk::DeviceSize                  size)
            : BufferData(physical_device,
                         device,
                         size,
                         vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
                             vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                         vk::MemoryPropertyFlagBits::eDeviceLocal)
        {}
    };

} // namespace Meow
