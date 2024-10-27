#include "editor.h"

#include "meow_runtime/runtime.h"
#include <iostream>

namespace Meow
{
    bool MeowEditor::Init() { return MeowRuntime::Get().Init(); }

    bool MeowEditor::Start() { return MeowRuntime::Get().Start(); }

    void MeowEditor::Tick(float dt) { MeowRuntime::Get().Tick(dt); }

    void MeowEditor::ShutDown() { MeowRuntime::Get().ShutDown(); }

    bool MeowEditor::IsRunning() { return m_running && MeowRuntime::Get().IsRunning(); }
    void MeowEditor::SetRunning(bool running) { m_running = running; }
} // namespace Meow