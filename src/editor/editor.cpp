#include "editor.h"

#include "runtime/core/time/time.h"
#include "runtime/runtime.h"

#include <iostream>

namespace Meow
{
    bool MeowEditor::Init() { return MeowRuntime::Get().Init(); }

    bool MeowEditor::Start() { return MeowRuntime::Get().Start(); }

    void MeowEditor::Tick()
    {
        float dt = Time::Get().GetDeltaTime();
        std::cout << "dt = " << dt << std::endl;
        MeowRuntime::Get().Tick(dt);
        Time::Get().Update();
    }

    void MeowEditor::ShutDown() { MeowRuntime::Get().ShutDown(); }

    bool MeowEditor::IsRunning() { return m_running && MeowRuntime::Get().IsRunning(); }
    void MeowEditor::SetRunning(bool running) { m_running = running; }
} // namespace Meow