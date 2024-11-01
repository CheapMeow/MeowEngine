#include "game.h"
#include "meow_runtime/core/base/log.hpp"
#include "meow_runtime/function/global/runtime_context.h"

using namespace Meow;

int main()
{
    if (!MeowGame::Get().Init())
    {
        return 1;
    }

    MeowGame::Get().Start();

    while (MeowGame::Get().IsRunning())
    {
        MeowGame::Get().Tick(g_runtime_context.time_system->GetDeltaTime());
    }

    MeowGame::Get().ShutDown();
}