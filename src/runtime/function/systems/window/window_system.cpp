#include "window_system.h"

#include "function/global/runtime_global_context.h"

namespace Meow
{
    WindowSystem::WindowSystem()
    {
        m_window = std::make_shared<Window>(0);
        m_window->OnClose().connect([&]() { g_runtime_global_context.running = false; });
    }

    WindowSystem::~WindowSystem() { m_window = nullptr; }

    void WindowSystem::Update(float frame_time) { m_window->Update(frame_time); }
} // namespace Meow
