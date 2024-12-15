#include "window.h"
#include "core/math/math.h"

#include <iostream>
#include <stdexcept>

namespace Meow
{
    static_assert(GLFW_KEY_LAST == static_cast<int16_t>(KeyCode::Menu),
                  "GLFW keys count does not match our keys enum count.");
    static_assert(GLFW_MOUSE_BUTTON_LAST == static_cast<int16_t>(MouseButtonCode::ButtonLast),
                  "GLFW mouse button count does not match our mouse button enum count.");

    void CallbackWindowPosition(GLFWwindow* glfwWindow, int32_t xpos, int32_t ypos)
    {
        auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        if (window->m_fullscreen)
            return;

        window->m_position = {xpos, ypos};
        window->m_on_position_signal(window->m_position);
    }

    void CallbackWindowSize(GLFWwindow* glfwWindow, int32_t width, int32_t height)
    {
        if (width <= 0 || height <= 0)
            return;
        auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

        if (window->m_fullscreen)
        {
            window->m_fullscreen_size = {width, height};
            window->m_on_size_signal(window->m_fullscreen_size);
        }
        else
        {
            window->m_size = {width, height};
            window->m_on_size_signal(window->m_size);
        }
    }

    void CallbackWindowClose(GLFWwindow* glfwWindow)
    {
        auto window      = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        window->m_closed = false;
        // Engine::Get()->RequestClose();
        window->m_on_close_signal();
    }

    void CallbackWindowFocus(GLFWwindow* glfwWindow, int32_t focused)
    {
        auto window       = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        window->m_focused = static_cast<bool>(focused);
        window->m_on_focus_signal(focused == GLFW_TRUE);
    }

    void CallbackWindowIconify(GLFWwindow* glfwWindow, int32_t iconified)
    {
        auto window         = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        window->m_iconified = iconified;
        window->m_on_iconify_signal(iconified);
    }

    void CallbackFramebufferSize(GLFWwindow* glfwWindow, int32_t width, int32_t height)
    {
        auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        if (window->m_fullscreen)
            window->m_fullscreen_size = {width, height};
        else
            window->m_size = {width, height};
        // TODO: inform graphics resize
    }

    void CallbackCursorEnter(GLFWwindow* glfwWindow, int32_t entered)
    {
        auto window                    = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        window->m_glfw_window_selected = entered == GLFW_TRUE;
        window->m_on_enter_signal(entered == GLFW_TRUE);
    }

    void CallbackDrop(GLFWwindow* glfwWindow, int32_t count, const char** paths)
    {
        auto                     window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        std::vector<std::string> files(static_cast<uint32_t>(count));
        for (uint32_t i = 0; i < static_cast<uint32_t>(count); i++)
            files[i] = paths[i];

        window->m_on_drop_signal(files);
    }

    void CallbackKey(GLFWwindow* glfwWindow, int32_t key, int32_t scancode, int32_t action, int32_t mods)
    {
        auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        window->m_on_key_signal(
            static_cast<KeyCode>(key), static_cast<InputAction>(action), static_cast<uint8_t>(mods));
    }

    void CallbackChar(GLFWwindow* glfwWindow, uint32_t codepoint)
    {
        auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        window->m_on_char_signal(static_cast<char>(codepoint));
    }

    void CallbackMouseButton(GLFWwindow* glfwWindow, int32_t button, int32_t action, int32_t mods)
    {
        auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        window->m_on_mouse_button_signal(
            static_cast<MouseButtonCode>(button), static_cast<InputAction>(action), static_cast<uint8_t>(mods));
    }

    void CallbackCursorPos(GLFWwindow* glfwWindow, double xpos, double ypos)
    {
        auto window              = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        window->m_mouse_position = {xpos, ypos};
        window->m_on_mouse_position_signal(window->m_mouse_position);
    }

    void CallbackScroll(GLFWwindow* glfwWindow, double xoffset, double yoffset)
    {
        auto window            = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
        window->m_mouse_scroll = {yoffset, yoffset};
        window->m_on_mouse_scroll_signal(window->m_mouse_scroll);
    }

    double Window::SmoothScrollWheel(double value, float delta)
    {
        if (value != 0.0)
        {
            value -= static_cast<double>(delta) * std::copysign(3.0, value);
            value = Math::Deadband(0.08, value);
            return value;
        }

        return 0.0;
    }

    Window::Window(std::size_t id, GLFWwindow* glfw_window)
        : m_glfw_window_id(id)
        , m_size(1080, 720)
        , m_title("Meow Engine Window")
        , m_resizable(true)
        , m_focused(true)
    {
        if (!glfw_window)
        {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_RESIZABLE, true);
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

            // Create a windowed mode window and its context.
            m_glfw_window = glfwCreateWindow(1080, 720, "Meow Engine GLFW Window", NULL, NULL);

            // Gets any window errors.
            if (!m_glfw_window)
            {
                glfwTerminate();
                throw std::runtime_error("GLFW failed to create the window");
            }
        }
        else
        {
            m_glfw_window = glfw_window;
        }

        glfwMakeContextCurrent(m_glfw_window);
        glfwSwapInterval(1);

        // Sets the user pointer.
        glfwSetWindowUserPointer(m_glfw_window, this);

        // Window attributes that can change later.
        glfwSetWindowAttrib(m_glfw_window, GLFW_DECORATED, !m_borderless);
        glfwSetWindowAttrib(m_glfw_window, GLFW_RESIZABLE, m_resizable);
        glfwSetWindowAttrib(m_glfw_window, GLFW_FLOATING, m_floating);

        // Centre the window position.
        // position.x = (videoMode.width - size.x) / 2;
        // position.y = (videoMode.height - size.y) / 2;
        // glfwSetWindowPos(window, position.x, position.y);

        // Sets fullscreen if enabled.
        // if (m_fullscreen)
        //    SetFullscreen(true);

        // Shows the glfw window.
        glfwShowWindow(m_glfw_window);

        // Sets the displays callbacks.
        glfwSetWindowPosCallback(m_glfw_window, CallbackWindowPosition);
        glfwSetWindowSizeCallback(m_glfw_window, CallbackWindowSize);
        glfwSetWindowCloseCallback(m_glfw_window, CallbackWindowClose);
        glfwSetWindowFocusCallback(m_glfw_window, CallbackWindowFocus);
        glfwSetWindowIconifyCallback(m_glfw_window, CallbackWindowIconify);
        glfwSetFramebufferSizeCallback(m_glfw_window, CallbackFramebufferSize);
        glfwSetCursorEnterCallback(m_glfw_window, CallbackCursorEnter);
        glfwSetDropCallback(m_glfw_window, CallbackDrop);
        glfwSetKeyCallback(m_glfw_window, CallbackKey);
        glfwSetCharCallback(m_glfw_window, CallbackChar);
        glfwSetMouseButtonCallback(m_glfw_window, CallbackMouseButton);
        glfwSetCursorPosCallback(m_glfw_window, CallbackCursorPos);
        glfwSetScrollCallback(m_glfw_window, CallbackScroll);
    }

    Window::~Window()
    {
        // Free the window callbacks and destroy the window.
        glfwDestroyWindow(m_glfw_window);

        m_closed = true;
    }

    void Window::Tick(float dt)
    {
        glfwSwapBuffers(m_glfw_window);
        glfwPollEvents();

        // Updates the position delta.
        m_mouse_position_delta = dt * (m_mouse_last_position - m_mouse_position);
        m_mouse_last_position  = m_mouse_position;

        // Updates the scroll delta.
        m_mouse_scroll_delta = dt * (m_mouse_last_scroll - m_mouse_scroll);
        m_mouse_last_scroll  = m_mouse_scroll;
    }

    void Window::SetSize(const glm::ivec2& size)
    {
        if (size.x != -1)
            m_size.x = size.x;
        if (size.y != -1)
            m_size.y = size.y;
        glfwSetWindowSize(m_glfw_window, size.x, size.y);
    }

    void Window::SetPosition(const glm::ivec2& position)
    {
        if (position.x != -1)
            m_position.x = position.x;
        if (position.x != -1)
            m_position.y = position.y;
        glfwSetWindowPos(m_glfw_window, position.x, position.y);
    }

    void Window::SetTitle(const std::string& title)
    {
        m_title = title;
        glfwSetWindowTitle(m_glfw_window, title.c_str());
        m_on_title_signal(title);
    }

    void Window::SetBorderless(bool borderless)
    {
        m_borderless = borderless;
        glfwSetWindowAttrib(m_glfw_window, GLFW_DECORATED, !borderless);
        m_on_borderless_signal(borderless);
    }

    void Window::SetResizable(bool resizable)
    {
        m_resizable = resizable;
        glfwSetWindowAttrib(m_glfw_window, GLFW_RESIZABLE, resizable);
        m_on_resizable_signal(resizable);
    }

    void Window::SetFloating(bool floating)
    {
        m_floating = floating;
        glfwSetWindowAttrib(m_glfw_window, GLFW_FLOATING, floating);
        m_on_floating_signal(floating);
    }

    void Window::SetIconified(bool iconify)
    {
        if (!m_iconified && iconify)
        {
            glfwIconifyWindow(m_glfw_window);
        }
        else if (m_iconified && !iconify)
        {
            glfwRestoreWindow(m_glfw_window);
        }
    }

    std::string Window::GetClipboard() const { return glfwGetClipboardString(m_glfw_window); }

    void Window::SetClipboard(const std::string& string) const
    {
        glfwSetClipboardString(m_glfw_window, string.c_str());
    }

    void Window::SetCursorHidden(bool hidden)
    {
        if (m_cursor_hidden != hidden)
        {
            glfwSetInputMode(m_glfw_window, GLFW_CURSOR, hidden ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

            if (!hidden && m_cursor_hidden)
                SetMousePosition(m_mouse_position);
        }

        m_cursor_hidden = hidden;
    }

    InputAction Window::GetKeyAction(KeyCode key) const
    {
        auto state = glfwGetKey(m_glfw_window, static_cast<int32_t>(key));
        return static_cast<InputAction>(state);
    }

    InputAction Window::GetMouseButtonAction(MouseButtonCode mouseButton) const
    {
        auto state = glfwGetMouseButton(m_glfw_window, static_cast<int32_t>(mouseButton));
        return static_cast<InputAction>(state);
    }

    void Window::SetMousePosition(const glm::vec2& mousePosition)
    {
        m_mouse_last_position = mousePosition;
        m_mouse_position      = mousePosition;
        glfwSetCursorPos(m_glfw_window, mousePosition.x, mousePosition.y);
    }

    void Window::SetMouseScroll(const glm::vec2& mouseScroll)
    {
        m_mouse_last_scroll = mouseScroll;
        m_mouse_scroll      = mouseScroll;
    }
} // namespace Meow
