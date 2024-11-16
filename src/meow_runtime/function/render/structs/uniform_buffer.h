#pragma once

#include "buffer_data.h"
#include "core/base/alignment.h"
#include "core/base/non_copyable.h"

namespace Meow
{
    struct UniformBuffer : BufferData
    {
        uint8_t* mapped_data_ptr = nullptr;

        uint64_t allocated_memory = 0;
        uint32_t min_alignment    = 0;

        UniformBuffer(std::nullptr_t)
            : BufferData(nullptr)
        {}

        UniformBuffer(const vk::raii::PhysicalDevice& physical_device,
                      const vk::raii::Device&         device,
                      vk::DeviceSize                  size,
                      vk::BufferUsageFlags            usage,
                      vk::MemoryPropertyFlags         property_flags = vk::MemoryPropertyFlagBits::eHostVisible |
                                                               vk::MemoryPropertyFlagBits::eHostCoherent)
            : BufferData(physical_device, device, size, usage, property_flags)
        {
            min_alignment   = physical_device.getProperties().limits.minUniformBufferOffsetAlignment;
            mapped_data_ptr = (uint8_t*)device_memory.mapMemory(0, VK_WHOLE_SIZE);
        }

        ~UniformBuffer()
        {
            mapped_data_ptr = nullptr;
            device_memory.unmapMemory();
        }

        void Reset();

        uint64_t AllocateMemory(uint64_t size);

        uint64_t Populate(void* src, uint64_t size);
    };
} // namespace Meow