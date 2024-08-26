#include "ring_uniform_buffer.h"

#include "pch.h"

namespace Meow
{
    RingUniformBuffer::RingUniformBuffer(vk::raii::PhysicalDevice const& physical_device,
                                         vk::raii::Device const&         logical_device)
    {
        buffer_size   = 32 * 1024; // 32KB
        min_alignment = physical_device.getProperties().limits.minUniformBufferOffsetAlignment;

        buffer_data_ptr = std::make_shared<BufferData>(physical_device,
                                                       logical_device,
                                                       buffer_size,
                                                       vk::BufferUsageFlagBits::eUniformBuffer |
                                                           vk::BufferUsageFlagBits::eTransferDst);
        mapped_data_ptr = buffer_data_ptr->device_memory.mapMemory(0, VK_WHOLE_SIZE);
    }

    RingUniformBuffer::~RingUniformBuffer()
    {
        mapped_data_ptr = nullptr;
        if (buffer_data_ptr != nullptr)
            buffer_data_ptr->device_memory.unmapMemory();
        buffer_data_ptr = nullptr;
    }

    uint64_t RingUniformBuffer::AllocateMemory(uint64_t size)
    {
        uint64_t new_memory_start = Align<uint64_t>(allocated_memory, min_alignment);

        if (new_memory_start + size <= buffer_size)
        {
            allocated_memory = new_memory_start + size;

            // stat
            begin = new_memory_start;
            usage = size;

            return new_memory_start;
        }

        // TODO: Scalable buffer size
        // When the memory is running out, allocation return to the begin of memory
        // It means overriding the old buffer data
        // If the object number is very large, it definitely break the uniform data at this frame
        allocated_memory = size;

        // stat
        begin = 0;
        usage = size;

        return 0;
    }
} // namespace Meow