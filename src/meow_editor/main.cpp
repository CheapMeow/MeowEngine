#include "editor.h"
#include "meow_runtime/core/base/log.hpp"
#include "meow_runtime/function/global/runtime_context.h"

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
        MeowEditor::Get().Tick(g_runtime_context.time_system->GetDeltaTime());
    }

    MeowEditor::Get().ShutDown();
}