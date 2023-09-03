#include "engine.h"

#include <iostream>

namespace Meow
{
    bool MeowEngine::init()
    {
        std::cout << "Hello world!" << std::endl;
        return true;
    }

    void MeowEngine::run()
    {
        while (1)
            ;
    }

    void MeowEngine::shutdown() {}
} // namespace Meow