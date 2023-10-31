#pragma once

#include "function/ecs/system.h"

namespace Meow
{
    class CameraSystem : System
    {
    public:
        void Update(float frame_time);
    };
} // namespace Meow