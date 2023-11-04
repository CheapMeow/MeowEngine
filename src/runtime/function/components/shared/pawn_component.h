#pragma once

#include "function/components/component.h"

namespace Meow
{
    struct PawnComponent : Component
    {
        bool is_player = false;
    };

} // namespace Meow
