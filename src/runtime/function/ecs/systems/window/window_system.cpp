#include "window_system.h"

#include "function/global/runtime_global_context.h"
#include "function/renderer/window.h"

namespace Meow
{
    WindowSystem::WindowSystem()
    {
        g_runtime_global_context.window = std::make_shared<Window>(0);
        g_runtime_global_context.window->OnClose().connect([&]() { g_runtime_global_context.running = false; });
    }

    WindowSystem::~WindowSystem() { g_runtime_global_context.window = nullptr; }

    void WindowSystem::Update(float frame_time) { g_runtime_global_context.window->Update(frame_time); }
} // namespace Meow
