#pragma once

#include "core/base/macro.h"
#include "function/system.h"
#include "function/window/window.h"

#include <memory>

namespace Meow
{
    class LIBRARY_API WindowSystem final : public System
    {
    public:
        WindowSystem();
        ~WindowSystem();

        void Start() override;

        void Tick(float dt) override;

        void AddWindow(std::shared_ptr<Window> window)
        {
            m_current_window      = window;
            m_current_glfw_window = window->GetGLFWWindow();
        }

        std::shared_ptr<Window> GetCurrentFocusWindow() { return m_current_window; }

        GLFWwindow* GetCurrentFocusGLFWWindow() { return m_current_glfw_window; }

    private:
        GLFWwindow*             m_current_glfw_window = nullptr;
        std::shared_ptr<Window> m_current_window      = nullptr;
    };
} // namespace Meow
