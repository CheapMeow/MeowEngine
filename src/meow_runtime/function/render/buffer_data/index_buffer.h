#pragma once

#include "buffer_data.h"

#include <memory>

namespace Meow
{
    struct IndexBuffer : public BufferData
    {
        size_t        instance_count = 1;
        size_t        index_count    = 0;
        vk::IndexType index_type     = vk::IndexType::eUint32;

        IndexBuffer(vk::raii::PhysicalDevice const& physical_device,
                    vk::raii::Device const&         device,
                    vk::raii::CommandPool const&    command_pool,
                    vk::raii::Queue const&          queue,
                    std::vector<uint32_t>&          indices)
            : BufferData(physical_device,
                         device,
                         indices.size() * sizeof(uint32_t),
                         vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                         vk::MemoryPropertyFlagBits::eDeviceLocal)
        {
            index_count = indices.size();

            Upload(physical_device, device, command_pool, queue, indices, 0);
        }
    };
} // namespace Meow
