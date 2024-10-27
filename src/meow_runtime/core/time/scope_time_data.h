#pragma once

#include <chrono>
#include <string>
#include <thread>

namespace Meow
{
    struct ScopeTimeData
    {
        std::string               name;
        std::string               filename;
        std::chrono::microseconds start;
        std::chrono::microseconds duration;
        int                       depth;
        std::thread::id           thread_id;
    };
} // namespace Meow