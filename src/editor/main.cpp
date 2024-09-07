#include "editor.h"
#include "runtime/core/base/log.hpp"
#include "runtime/core/time/time.h"
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
        MeowEditor::Get().Tick();
    }

    MeowEditor::Get().ShutDown();
}