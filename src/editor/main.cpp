#include "engine.h"

using namespace Meow;

int main()
{
    MeowEngine engine;
    if (!engine.init())
    {
        return 1;
    }

    engine.run();

    engine.shutdown();
}