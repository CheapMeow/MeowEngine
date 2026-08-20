#include "editor.h"

#include "global/editor_context.h"
#include "meow_runtime/function/global/runtime_context.h"
#include "meow_runtime/runtime.h"
#include "render/editor_window.h"

#include <iostream>

namespace Meow
{
    bool MeowEditor::Init()
    {
        if (!MeowRuntime::Get().Init())
            return false;

        g_editor_context.profile_system = std::make_shared<ProfileSystem>();

        return true;
    }

    bool MeowEditor::Start()
    {
        if (!MeowRuntime::Get().Start())
            return false;

        g_editor_context.profile_system->Start();

        g_runtime_context.window_system->AddWindow(
            std::make_shared<EditorWindow>(0, g_runtime_context.window_system->GetCurrentFocusGLFWWindow()));
        g_runtime_context.input_system->BindDefault(g_runtime_context.window_system->GetCurrentFocusWindow());

        // run JS scene init script (needs window's render pass to be ready)
        if (g_runtime_context.js_system)
            g_runtime_context.js_system->LoadScript(ENGINE_ROOT_DIR "/scripts/init_scene.js");

        return true;
    }

    void MeowEditor::Tick(float dt)
    {
        MeowRuntime::Get().Tick(dt);

        g_editor_context.profile_system->Tick(dt);
    }

    void MeowEditor::Shutdown()
    {
        g_editor_context.profile_system = nullptr;

        MeowRuntime::Get().Shutdown();
    }

    bool MeowEditor::IsRunning() { return m_running && MeowRuntime::Get().IsRunning(); }
    void MeowEditor::SetRunning(bool running) { m_running = running; }
} // namespace Meow