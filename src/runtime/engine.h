#pragma once

#include "core/base/macro.h"
#include "core/base/non_copyable.h"
#include "core/window/window.h"

#include <memory>
namespace Meow
{
    /**
     * @brief Engine entry.
     */
    class LIBRARY_API MeowEngine : NonCopyable
    {
    public:
        bool Init(); /**< Init engine */
        void Run();
        void ShutDown();

    private:
        bool m_Running = true;

        double m_LastTime = 0.0;
        // double m_Accumulator          = 0.0;
        // double m_PhyicsTime           = 0.0;
        // double m_PhyicsFixedDeltaTime = 1.0 / 60.0;

        std::unique_ptr<Window> m_Window;
    };
} // namespace Meow