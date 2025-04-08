#pragma once

#include "buffer_data.h"

#include <memory>

namespace Meow
{
    struct StorageBuffer
    {
        std::shared_ptr<BufferData> buffer_data_ptr = nullptr;
        VkDeviceSize                offset          = 0;

        StorageBuffer(vk::raii::PhysicalDevice const& physical_device,
                      vk::raii::Device const&         device,
                      vk::raii::CommandPool const&    command_pool,
                      vk::raii::Queue const&          queue,
                      std::vector<float>&             data)
        {
            buffer_data_ptr = std::make_shared<BufferData>(physical_device,
                                                           device,
                                                           data.size() * sizeof(float),
                                                           vk::BufferUsageFlagBits::eStorageBuffer |
                                                               vk::BufferUsageFlagBits::eStorageBuffer |
                                                               vk::BufferUsageFlagBits::eTransferDst,
                                                           vk::MemoryPropertyFlagBits::eDeviceLocal);
            buffer_data_ptr->Upload(physical_device, device, command_pool, queue, data, 0);
        }
    };

} // namespace Meow
