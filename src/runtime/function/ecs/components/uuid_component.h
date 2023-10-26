#pragma once

#include "function/ecs/component.h"

#include <cstdint>

namespace Meow
{
    class UUIDComponent : Component
    {
    public:
        uint32_t m_uuid;
    };

} // namespace Meow