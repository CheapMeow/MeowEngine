#include "render_system.h"

#include "function/global/runtime_global_context.h"

namespace Meow
{
    RenderSystem::RenderSystem() { m_renderer = std::make_unique<VulkanRenderer>(); }

    void RenderSystem::Update(float frame_time) { m_renderer->Update(frame_time); }
} // namespace Meow
