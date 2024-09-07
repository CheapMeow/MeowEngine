#pragma once

#include "core/base/macro.h"

#include <chrono>
#include <iostream>

namespace Meow
{
    class LIBRARY_API Time
    {
    public:
        Time(const Time&) = delete;
        Time(Time&&)      = delete;

        friend class MeowRuntime;
        friend class MeowEditor;

        static Time& Get()
        {
            static Time instance;
            return instance;
        }

        float GetDeltaTime() { return m_elapsed_time; }

    private:
        Time() { m_last_timepoint = std::chrono::steady_clock::now(); }

        void Update()
        {
            auto end_timepoint = std::chrono::steady_clock::now();
            auto elapsed_time =
                std::chrono::time_point_cast<std::chrono::microseconds>(end_timepoint).time_since_epoch() -
                std::chrono::time_point_cast<std::chrono::microseconds>(m_last_timepoint).time_since_epoch();
            m_last_timepoint = end_timepoint;
            m_elapsed_time   = (float)elapsed_time.count() / 1000000.0;
        }

        std::chrono::time_point<std::chrono::steady_clock> m_last_timepoint;
        float                                              m_elapsed_time;
    };
} // namespace Meow