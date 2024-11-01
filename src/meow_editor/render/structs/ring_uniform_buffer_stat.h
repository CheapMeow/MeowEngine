#pragma once

#include "meow_runtime/function/render/structs/ring_uniform_buffer.h"

#include <cstdint>

namespace Meow
{
    struct RingUniformBufferStat
    {
        uint64_t begin = 0;
        uint64_t usage = 0;
        uint64_t size  = 0;

        void populate(RingUniformBuffer const& ring_buf)
        {
            begin = ring_buf.begin;
            usage = ring_buf.usage;
            size  = ring_buf.buffer_size;
        }
    };
} // namespace Meow