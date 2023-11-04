#pragma once

#include "function/components/component.h"

#include <cstdint>

namespace Meow
{
    struct UUIDComponent : Component
    {
        uint32_t m_uuid;
    };

} // namespace Meow