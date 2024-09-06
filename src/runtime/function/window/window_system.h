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

        void AddWindow(std::shared_ptr<Window> window) { m_window = window; }

        std::shared_ptr<Window> GetCurrentFocusWindow() { return m_window; }

    private:
        std::shared_ptr<Window> m_window;
    };
} // namespace Meow
