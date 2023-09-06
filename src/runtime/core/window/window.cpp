#include "window.h"

#include <stdexcept>

namespace Meow
{
    Window::Window(std::size_t id)
    {
        glfwInit();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

        // Create a windowed mode window and its context.
        m_window = glfwCreateWindow(1080, 720, "Test Window", NULL, NULL);

        // Gets any window errors.
        if (!m_window)
        {
            glfwTerminate();
            throw std::runtime_error("GLFW failed to create the window");
        }

        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(1);

        // Sets the user pointer.
        glfwSetWindowUserPointer(m_window, this);

        // Shows the glfw window.
        glfwShowWindow(m_window);
    }

    Window::~Window()
    {
        // Free the window callbacks and destroy the window.
        glfwDestroyWindow(m_window);
    }
} // namespace Meow
