#pragma once

#include "buffer_data.h"

#include <memory>

namespace Meow
{
    struct IndexBuffer : public BufferData
    {
        size_t        instance_count = 1;
        vk::IndexType index_type     = vk::IndexType::eUint32;

        IndexBuffer(vk::raii::PhysicalDevice const& physical_device,
                    vk::raii::Device const&         device,
                    vk::raii::CommandPool const&    command_pool,
                    vk::raii::Queue const&          queue,
                    vk::DeviceSize                  size)
            : BufferData(physical_device,
                         device,
                         size,
                         vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                         vk::MemoryPropertyFlagBits::eDeviceLocal)
        {}
    };
} // namespace Meow
