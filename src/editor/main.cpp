#include "runtime/core/log/log.h"
#include "runtime/engine.h"

using namespace Meow;

int main()
{
    if (!MeowEngine::GetEngine().Init())
    {
        return 1;
    }

    MeowEngine::GetEngine().Start();

    EDITOR_INFO("Editor is running!");
    MeowEngine::GetEngine().Run();

    MeowEngine::GetEngine().ShutDown();
}