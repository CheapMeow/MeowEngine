#include "runtime/core/base/log.h"
#include "runtime/engine.h"

using namespace Meow;

int main()
{
    MeowEngine engine;
    if (!engine.init())
    {
        return 1;
    }

    EDITOR_INFO("Editor is running!");
    engine.run();

    engine.shutdown();
}