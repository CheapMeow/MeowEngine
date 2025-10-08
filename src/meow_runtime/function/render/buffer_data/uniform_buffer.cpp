#include "uniform_buffer.h"

#include "pch.h"

namespace Meow
{
    void UniformBuffer::ResetMemory() { allocated_memory = 0; }

    uint64_t UniformBuffer::AllocateMemory(uint64_t size)
    {
        uint64_t new_memory_start = Align<uint64_t>(allocated_memory, min_alignment);

        if (new_memory_start + size <= device_size)
        {
            allocated_memory = new_memory_start + size;

            return new_memory_start;
        }

        // TODO: Scalable buffer size
        // When the memory is running out, allocation return to the begin of memory
        // It means overriding the old buffer data
        // If the object number is very large, it definitely break the uniform data at this frame
        // Here we push the problem aside

        allocated_memory = size;
        return 0;
    }

    uint64_t UniformBuffer::Populate(void* src, uint64_t size)
    {
        uint64_t new_memory_start = AllocateMemory(size);

        memcpy(mapped_data_ptr + new_memory_start, src, size);

        return new_memory_start;
    }

} // namespace Meow