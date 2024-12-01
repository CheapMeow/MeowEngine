#include "game.h"

#include "meow_runtime/function/global/runtime_context.h"
#include "meow_runtime/runtime.h"
#include "render/game_window.h"

#include <iostream>

namespace Meow
{
    bool MeowGame::Init() { return MeowRuntime::Get().Init(); }

    bool MeowGame::Start()
    {
        if (!MeowRuntime::Get().Start())
            return false;

        g_runtime_context.window_system->AddWindow(
            std::make_shared<GameWindow>(0, g_runtime_context.window_system->GetCurrentFocusGLFWWindow()));
        g_runtime_context.input_system->BindDefault(g_runtime_context.window_system->GetCurrentFocusWindow());

        return true;
    }

    void MeowGame::Tick(float dt) { MeowRuntime::Get().Tick(dt); }

    void MeowGame::ShutDown() { MeowRuntime::Get().ShutDown(); }

    bool MeowGame::IsRunning() { return m_running && MeowRuntime::Get().IsRunning(); }
    void MeowGame::SetRunning(bool running) { m_running = running; }
} // namespace Meow