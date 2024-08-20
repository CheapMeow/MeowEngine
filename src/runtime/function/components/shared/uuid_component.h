#pragma once

#include "function/components/component.h"

#include <cstdint>

namespace Meow
{
    class UUIDComponent : public Component
    {
    public:
        uint32_t m_uuid;
    };

} // namespace Meow