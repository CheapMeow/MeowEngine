#pragma once

#include "function/systems/system.h"

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
