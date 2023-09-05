#include "engine.h"
#include "core/base/log.h"

#include <iostream>

namespace Meow
{
    bool MeowEngine::init()
    {
        Log::Init();
        RUNTIME_INFO("Hello world!");
        return true;
    }

    void MeowEngine::run()
    {
        while (1)
            ;
    }

    void MeowEngine::shutdown() {}
} // namespace Meow