#include "engine.h"
#include "core/log/log.h"
#include "core/window/window.h"

#include <iostream>

namespace Meow
{
    bool MeowEngine::Init()
    {
        Log::Init();
        Window test_window(0);
        RUNTIME_INFO("Hello world!");
        return true;
    }

    void MeowEngine::Run()
    {
        while (1)
            ;
    }

    void MeowEngine::ShutDown() {}
} // namespace Meow