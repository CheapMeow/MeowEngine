#pragma once

#include "pch.h"

#include "core/base/macro.h"
#include "core/base/non_copyable.h"
#include "function/system.h"

#include <memory>
#include <vector>

namespace Meow
{
    /**
     * @brief Engine entry.
     */
    class LIBRARY_API MeowEngine : NonCopyable
    {
    public:
        bool Init(); /**< Init engine */
        bool Start();
        void Run();
        void ShutDown();

        static MeowEngine& GetEngine()
        {
            static MeowEngine instance;
            return instance;
        }

        void SetRunning(bool running) { m_running = running; }

    private:
        bool m_running = true;

        float m_last_time = 0.0;
        // float m_accumulator          = 0.0;
        // float m_phyics_time           = 0.0;
        // float m_phyics_fixed_dt = 1.0 / 60.0;
    };
} // namespace Meow