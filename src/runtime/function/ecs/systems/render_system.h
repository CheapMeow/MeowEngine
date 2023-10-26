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

        void Update(float frame_time);

    private:
        std::unique_ptr<VulkanRenderer> m_renderer;
    };
} // namespace Meow
