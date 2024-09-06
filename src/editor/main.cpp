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

    float last_time = 0.0;
    while (MeowEditor::Get().IsRunning())
    {
        float curr_time = Time::GetTime();
        float dt        = curr_time - last_time;
        last_time       = curr_time;

        MeowEditor::Get().Tick(dt);
    }

    MeowEditor::Get().ShutDown();
}