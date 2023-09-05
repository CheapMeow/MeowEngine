#include "runtime/core/log/log.h"
#include "runtime/engine.h"

using namespace Meow;

int main()
{
    MeowEngine engine;
    if (!engine.Init())
    {
        return 1;
    }

    EDITOR_INFO("Editor is running!");
    engine.Run();

    engine.ShutDown();
}