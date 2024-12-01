#include "runtime.h"

using namespace Meow;

int main()
{
    if (!MeowRuntime::Get().Init())
    {
        return 1;
    }

    MeowRuntime::Get().Start();

    while (MeowRuntime::Get().IsRunning())
    {
        MeowRuntime::Get().Tick();
    }

    MeowRuntime::Get().ShutDown();
}