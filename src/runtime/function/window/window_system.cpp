#include "window_system.h"

#include "pch.h"

#include "function/global/runtime_global_context.h"

namespace Meow
{
    WindowSystem::WindowSystem()
    {
        m_window = std::make_shared<Window>(0);
        m_window->OnClose().connect([&]() { g_runtime_global_context.running = false; });
        m_window->OnSize().connect(
            [&](glm::ivec2 new_size) { g_runtime_global_context.render_system->SetResized(true); });
    }

    WindowSystem::~WindowSystem() { m_window = nullptr; }

    void WindowSystem::Start() {}

    void WindowSystem::Tick(float dt)
    {
        FUNCTION_TIMER();

        m_window->Tick(dt);
    }
} // namespace Meow
