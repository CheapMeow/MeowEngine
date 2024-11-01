#pragma once

#include "buffer_data.h"
#include "core/base/alignment.h"
#include "core/base/non_copyable.h"

namespace Meow
{
    struct RingUniformBuffer : NonCopyable
    {
        std::shared_ptr<BufferData> buffer_data_ptr  = nullptr;
        void*                       mapped_data_ptr  = nullptr;
        uint64_t                    buffer_size      = 0;
        uint64_t                    allocated_memory = 0;
        uint32_t                    min_alignment    = 0;

        // stat
        uint64_t begin;
        uint64_t usage;

        RingUniformBuffer(std::nullptr_t) {}

        RingUniformBuffer(RingUniformBuffer&& rhs) noexcept
        {
            std::swap(buffer_data_ptr, rhs.buffer_data_ptr);
            std::swap(mapped_data_ptr, rhs.mapped_data_ptr);
            this->buffer_size      = rhs.buffer_size;
            this->allocated_memory = rhs.allocated_memory;
            this->min_alignment    = rhs.min_alignment;
        }

        RingUniformBuffer& operator=(RingUniformBuffer&& rhs) noexcept
        {
            if (this != &rhs)
            {
                std::swap(buffer_data_ptr, rhs.buffer_data_ptr);
                std::swap(mapped_data_ptr, rhs.mapped_data_ptr);
                this->buffer_size      = rhs.buffer_size;
                this->allocated_memory = rhs.allocated_memory;
                this->min_alignment    = rhs.min_alignment;
            }

            return *this;
        }

        RingUniformBuffer(vk::raii::PhysicalDevice const& physical_device, vk::raii::Device const& logical_device);

        ~RingUniformBuffer();

        uint64_t AllocateMemory(uint64_t size);
    };
} // namespace Meow