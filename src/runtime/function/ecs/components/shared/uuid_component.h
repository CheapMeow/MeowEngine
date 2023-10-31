#pragma once

#include "function/ecs/component.h"

#include <cstdint>

namespace Meow
{
    struct UUIDComponent : Component
    {
        uint32_t m_uuid;
    };

} // namespace Meow