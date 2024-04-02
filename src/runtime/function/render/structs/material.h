#pragma once

#include "buffer_data.h"
#include "core/base/alignment.h"
#include "shader.h"

#include <memory>

namespace Meow
{
    struct RingUniformBuffer
    {
        std::shared_ptr<BufferData> buffer_data_ptr  = nullptr;
        void*                       mapped_data_ptr  = nullptr;
        uint64_t                    buffer_size      = 0;
        uint64_t                    allocated_memory = 0;
        uint32_t                    min_alignment    = 0;

        RingUniformBuffer(vk::raii::PhysicalDevice const& physical_device,
                          vk::raii::Device const&         device,
                          vk::raii::CommandPool const&    command_pool,
                          vk::raii::Queue const&          queue,
                          vk::MemoryPropertyFlags         property_flags,
                          std::vector<float>&             vertices)
        {
            buffer_size   = 32 * 1024 * 1024; // 32MB
            min_alignment = physical_device.getProperties().limits.minUniformBufferOffsetAlignment;

            buffer_data_ptr = std::make_shared<BufferData>(physical_device,
                                                           device,
                                                           buffer_size,
                                                           vk::BufferUsageFlagBits::eUniformBuffer |
                                                               vk::BufferUsageFlagBits::eTransferDst,
                                                           property_flags);
            mapped_data_ptr = buffer_data_ptr->device_memory.mapMemory(0, VK_WHOLE_SIZE);
        }

        ~RingUniformBuffer()
        {
            mapped_data_ptr = nullptr;
            buffer_data_ptr->device_memory.unmapMemory();
        }

        uint64_t AllocateMemory(uint64_t size)
        {
            uint64_t new_memory_start = Align<uint64_t>(allocated_memory, min_alignment);

            if (new_memory_start + size <= buffer_size)
            {
                allocated_memory = new_memory_start + size;
                return new_memory_start;
            }

            // TODO: Scalable buffer size
            // When the memory is running out, allocation return to the begin of memory
            // It means overriding the old buffer data
            // If the object number is very large, it definitely break the uniform data at this frame
            allocated_memory = size;
            return 0;
        }
    };

    struct Material
    {
        Shader shader;
    };

} // namespace Meow
