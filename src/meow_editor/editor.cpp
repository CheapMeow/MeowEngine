#include "editor.h"

#include "meow_runtime/function/global/runtime_global_context.h"
#include "meow_runtime/runtime.h"
#include "render/editor_window.h"

#include <iostream>

namespace Meow
{
    bool MeowEditor::Init() { return MeowRuntime::Get().Init(); }

    bool MeowEditor::Start()
    {
        if (!MeowRuntime::Get().Start())
            return false;

        g_runtime_global_context.window_system->AddWindow(
            std::make_shared<EditorWindow>(0, g_runtime_global_context.window_system->GetCurrentFocusGLFWWindow()));
        g_runtime_global_context.input_system->BindDefault(
            g_runtime_global_context.window_system->GetCurrentFocusWindow());

        return true;
    }

    void MeowEditor::Tick(float dt) { MeowRuntime::Get().Tick(dt); }

    void MeowEditor::ShutDown() { MeowRuntime::Get().ShutDown(); }

    bool MeowEditor::IsRunning() { return m_running && MeowRuntime::Get().IsRunning(); }
    void MeowEditor::SetRunning(bool running) { m_running = running; }
} // namespace Meow