#include "window_system.h"

#include "pch.h"

#include "function/global/runtime_context.h"

namespace Meow
{
    WindowSystem::WindowSystem()
    {
        glfwInit();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_RESIZABLE, true);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // Create a windowed mode window and its context.
        m_current_glfw_window = glfwCreateWindow(1080, 720, "Meow Engine GLFW Window", NULL, NULL);

        // Gets any window errors.
        if (!m_current_glfw_window)
        {
            glfwTerminate();
            throw std::runtime_error("GLFW failed to create the window");
        }

        glfwMakeContextCurrent(m_current_glfw_window);
        glfwSwapInterval(1);

        // Sets the user pointer.
        glfwSetWindowUserPointer(m_current_glfw_window, this);

        // Window attributes that can change later.
        glfwSetWindowAttrib(m_current_glfw_window, GLFW_DECORATED, false);
        glfwSetWindowAttrib(m_current_glfw_window, GLFW_RESIZABLE, false);
        glfwSetWindowAttrib(m_current_glfw_window, GLFW_FLOATING, false);

        // Shows the glfw window.
        glfwShowWindow(m_current_glfw_window);
    }

    WindowSystem::~WindowSystem() {}

    void WindowSystem::Start() {}

    void WindowSystem::Tick(float dt)
    {
        FUNCTION_TIMER();

        if (m_current_window)
            m_current_window->Tick(dt);
    }
} // namespace Meow
