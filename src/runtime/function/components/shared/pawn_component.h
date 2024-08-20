#pragma once

#include "function/components/component.h"

namespace Meow
{
    class PawnComponent : public Component
    {
    public:
        bool is_player = false;
    };

} // namespace Meow
