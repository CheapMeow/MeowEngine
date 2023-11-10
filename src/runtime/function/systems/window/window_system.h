#pragma once

#include "function/systems/system.h"
#include "function/systems/window/window.h"

#include <memory>

namespace Meow
{
    class WindowSystem final : public System
    {
    public:
        WindowSystem();
        ~WindowSystem();

        void Update(float frame_time);

        std::shared_ptr<Window> m_window;
    };
} // namespace Meow
