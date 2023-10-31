#pragma once

#include "function/ecs/component.h"

namespace Meow
{
    struct PawnComponent : Component
    {
        bool is_player = false;
    };

} // namespace Meow
