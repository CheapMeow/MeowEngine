#include "time_system.h"

namespace Meow
{
    void TimeSystem::Start()
    {
        m_start_timepoint = std::chrono::steady_clock::now();
        m_last_timepoint  = std::chrono::steady_clock::now();
    }

    void TimeSystem::Tick(float dt)
    {
        auto end_timepoint = std::chrono::steady_clock::now();
        auto delta_time    = std::chrono::time_point_cast<std::chrono::microseconds>(end_timepoint).time_since_epoch() -
                          std::chrono::time_point_cast<std::chrono::microseconds>(m_last_timepoint).time_since_epoch();
        auto elapsed_time =
            std::chrono::time_point_cast<std::chrono::microseconds>(end_timepoint).time_since_epoch() -
            std::chrono::time_point_cast<std::chrono::microseconds>(m_start_timepoint).time_since_epoch();

        m_last_timepoint = end_timepoint;

        m_elapsed_time = elapsed_time.count() / 1000000.0;
        m_dt           = (float)delta_time.count() / 1000000.0;
    }
} // namespace Meow