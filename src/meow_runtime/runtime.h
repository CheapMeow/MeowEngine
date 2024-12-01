#pragma once

#include "core/base/non_copyable.h"
#include "function/system.h"

#include <memory>
#include <vector>

namespace Meow
{
    /**
     * @brief Engine entry.
     */
    class MeowRuntime : NonCopyable
    {
    public:
        bool Init();
        bool Start();
        void Tick();
        void ShutDown();

        static MeowRuntime& Get()
        {
            static MeowRuntime instance;
            return instance;
        }

        bool IsRunning() { return m_running; }
        void SetRunning(bool running) { m_running = running; }

    private:
        bool m_running = true;
    };
} // namespace Meow