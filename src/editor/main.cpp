#include "editor.h"
#include "runtime/core/base/log.hpp"
#include "runtime/function/global/runtime_global_context.h"

using namespace Meow;

int main()
{
    if (!MeowEditor::Get().Init())
    {
        return 1;
    }

    MeowEditor::Get().Start();

    while (MeowEditor::Get().IsRunning())
    {
        MeowEditor::Get().Tick(g_runtime_global_context.time_system->GetDeltaTime());
    }

    MeowEditor::Get().ShutDown();
}