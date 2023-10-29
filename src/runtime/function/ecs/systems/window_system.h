#pragma once

#include "function/ecs/system.h"

namespace Meow
{
    class WindowSystem final : public System
    {
    public:
        WindowSystem();
        ~WindowSystem();

        void Update(float frame_time);
    };
} // namespace Meow
