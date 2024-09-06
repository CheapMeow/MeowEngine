#include "window_system.h"

#include "pch.h"

#include "function/global/runtime_global_context.h"

namespace Meow
{
    WindowSystem::WindowSystem() {}

    WindowSystem::~WindowSystem() { m_window = nullptr; }

    void WindowSystem::Start() {}

    void WindowSystem::Tick(float dt)
    {
        FUNCTION_TIMER();

        if (m_window)
            m_window->Tick(dt);
    }
} // namespace Meow
