#pragma once

#include "runtime/core/base/non_copyable.h"

#include <memory>
#include <vector>

namespace Meow
{
    /**
     * @brief Engine entry.
     */
    class MeowEditor : NonCopyable
    {
    public:
        bool Init();
        bool Start();
        void Tick(float dt);
        void ShutDown();

        static MeowEditor& Get()
        {
            static MeowEditor instance;
            return instance;
        }

        bool IsRunning();
        void SetRunning(bool running);

    private:
        bool m_running = true;
    };
} // namespace Meow