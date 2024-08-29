#include "runtime/core/base/log.hpp"
#include "runtime/engine.h"

using namespace Meow;

int main()
{
    if (!MeowEngine::GetEngine().Init())
    {
        return 1;
    }

    MeowEngine::GetEngine().Start();

    MEOW_INFO("Editor is running!");
    MeowEngine::GetEngine().Run();

    MeowEngine::GetEngine().ShutDown();
}