#pragma once

#include "function/object/game_object.h"

namespace Meow
{
    class PawnComponent : public Component
    {
    public:
        bool is_player = false;
    };

} // namespace Meow
