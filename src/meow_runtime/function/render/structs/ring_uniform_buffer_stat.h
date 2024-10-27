#pragma once

#include <cstdint>

namespace Meow
{
    struct RingUniformBufferStat
    {
        uint64_t begin = 0;
        uint64_t usage = 0;
        uint64_t size  = 0;
    };
} // namespace Meow