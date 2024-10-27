#pragma once

#include "function/system.h"

#include <chrono>

namespace Meow
{
    class TimeSystem : public System
    {
    public:
        void Start() override;

        void Tick(float dt) override;

        float GetTime() const { return m_elapsed_time; }
        float GetDeltaTime() const { return m_dt; }

    private:
        float m_elapsed_time;
        float m_dt;

        std::chrono::time_point<std::chrono::steady_clock> m_start_timepoint;
        std::chrono::time_point<std::chrono::steady_clock> m_last_timepoint;
    };
} // namespace Meow
