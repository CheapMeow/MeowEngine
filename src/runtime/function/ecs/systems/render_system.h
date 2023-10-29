#pragma once

#include "function/ecs/system.h"
#include "function/renderer/vulkan_renderer.h"
#include "function/renderer/window.h"

namespace Meow
{
    class RenderSystem final : public System
    {
    public:
        RenderSystem();
        ~RenderSystem();

        void Update(float frame_time);
    };
} // namespace Meow
