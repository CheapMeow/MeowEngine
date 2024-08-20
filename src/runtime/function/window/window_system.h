#pragma once

#include "function/system.h"
#include "function/window/window.h"

#include <memory>

namespace Meow
{
    class WindowSystem final : public System
    {
    public:
        WindowSystem();
        ~WindowSystem();

        void Start() override;

        void Tick(float dt) override;

        std::shared_ptr<Window> m_window;
    };
} // namespace Meow
