#pragma once

#include "buffer_data.h"

#include <memory>

namespace Meow
{
    struct IndexBuffer
    {
        std::shared_ptr<BufferData> buffer_data_ptr = nullptr;
        size_t                      instance_count  = 1;
        size_t                      index_count     = 0;
        vk::IndexType               index_type      = vk::IndexType::eUint32;

        IndexBuffer(vk::raii::PhysicalDevice const& physical_device,
                    vk::raii::Device const&         device,
                    vk::raii::CommandPool const&    command_pool,
                    vk::raii::Queue const&          queue,
                    vk::MemoryPropertyFlags         property_flags,
                    std::vector<uint32_t>&          indices)
        {
            index_count = indices.size();

            buffer_data_ptr = std::make_shared<BufferData>(physical_device,
                                                           device,
                                                           indices.size() * sizeof(uint32_t),
                                                           vk::BufferUsageFlagBits::eIndexBuffer |
                                                               vk::BufferUsageFlagBits::eTransferDst,
                                                           property_flags);
            buffer_data_ptr->Upload(physical_device, device, command_pool, queue, indices, 0);
        }
    };
} // namespace Meow
