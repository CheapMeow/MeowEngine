#include "window_system.h"

#include "pch.h"

#include "function/global/runtime_global_context.h"
#include "function/render/window/runtime_window.h"

namespace Meow
{
    WindowSystem::WindowSystem() {}

    WindowSystem::~WindowSystem() { m_window = nullptr; }

    void WindowSystem::Start() { AddWindow(std::make_shared<RuntimeWindow>(0)); }

    void WindowSystem::Tick(float dt)
    {
        FUNCTION_TIMER();

        if (m_window)
            m_window->Tick(dt);
    }
} // namespace Meow
