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
        m_Window = glfwCreateWindow(1080, 720, "Test Window", NULL, NULL);

        // Gets any window errors.
        if (!m_Window)
        {
            glfwTerminate();
            throw std::runtime_error("GLFW failed to create the window");
        }

        glfwMakeContextCurrent(m_Window);
        glfwSwapInterval(1);

        // Sets the user pointer.
        glfwSetWindowUserPointer(m_Window, this);

        // Shows the glfw window.
        glfwShowWindow(m_Window);
    }

    Window::~Window()
    {
        // Free the window callbacks and destroy the window.
        glfwDestroyWindow(m_Window);
    }

    void Window::Update(double dt)
    {
        glfwSwapBuffers(m_Window);
        glfwPollEvents();
    }
} // namespace Meow
