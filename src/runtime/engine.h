#pragma once

#include "core/base/macro.h"
#include "core/base/non_copyable.h"
#include "function/renderer/vulkan_renderer.h"
#include "function/renderer/window.h"

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
        bool m_running = true;

        double m_lastTime = 0.0;
        // double m_accumulator          = 0.0;
        // double m_phyics_time           = 0.0;
        // double m_phyics_fixed_delta_time = 1.0 / 60.0;

        std::shared_ptr<Window>         m_window;
        std::unique_ptr<VulkanRenderer> m_renderer;
    };
} // namespace Meow