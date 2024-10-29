#pragma once

#include "meow_runtime/core/base/non_copyable.h"

#include <memory>
#include <vector>

namespace Meow
{
    /**
     * @brief Game entry.
     */
    class MeowGame : public NonCopyable
    {
    public:
        bool Init();
        bool Start();
        void Tick(float dt);
        void ShutDown();

        static MeowGame& Get()
        {
            static MeowGame instance;
            return instance;
        }

        bool IsRunning();
        void SetRunning(bool running);

    private:
        bool m_running = true;
    };
} // namespace Meow