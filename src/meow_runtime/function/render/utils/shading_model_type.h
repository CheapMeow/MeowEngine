#pragma once

#include <stdint.h>

namespace Meow
{
    enum class ShadingModelType : uint8_t
    {
        Opaque      = 0,
        Translucent = 1,
    };
} // namespace Meow
