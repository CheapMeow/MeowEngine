#pragma once

#include "core/base/non_copyable.h"
#include "core/signal/signal.hpp"
#include "function/input/input_enum.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <string>

namespace Meow
{
    /**
     * @brief Window controls glfw window and input.
     */
    class Window : NonCopyable
    {
    public:
        Window(std::size_t id);
        virtual ~Window();

        virtual void Tick(float dt);

        /**
         * Gets the size of the window in pixels.
         * @param checkFullscreen If in fullscreen and true size will be the screens size.
         * @return The size of the window.
         */
        const glm::ivec2& GetSize(bool checkFullscreen = true) const
        {
            return (m_fullscreen && checkFullscreen) ? m_fullscreen_size : m_size;
        }

        /**
         * Sets the window size.
         * @param size The new size in pixels.
         */
        void SetSize(const glm::ivec2& size);

        /**
         * Gets the aspect ratio between the windows width and height.
         * @return The aspect ratio.
         */
        float GetAspectRatio() const { return static_cast<float>(GetSize().x) / static_cast<float>(GetSize().y); }

        /**
         * Gets the windows position in pixels.
         * @return The windows position.
         */
        const glm::ivec2& GetPosition() const { return m_position; }

        /**
         * Sets the window position to a new position in pixels.
         * @param position The new position in pixels.
         */
        void SetPosition(const glm::ivec2& position);

        /**
         * Gets the window's title.
         * @return The window's title.
         */
        const std::string& GetTitle() const { return m_title; }

        /**
         * Sets window title.
         * @param title The new title.
         */
        void SetTitle(const std::string& title);

        /**
         * Gets weather the window is borderless or not.
         * @return If the window is borderless.
         */
        bool IsBorderless() const { return m_borderless; }

        /**
         * Sets the window to be borderless.
         * @param borderless Weather or not to be borderless.
         */
        void SetBorderless(bool borderless);

        /**
         * Gets weather the window is resizable or not.
         * @return If the window is resizable.
         */
        bool IsResizable() const { return m_resizable; }

        /**
         * Sets the window to be resizable.
         * @param resizable Weather or not to be resizable.
         */
        void SetResizable(bool resizable);

        /**
         * Gets weather the window is floating or not, if floating the window will always display above other windows.
         * @return If the window is floating.
         */
        bool IsFloating() const { return m_floating; }

        /**
         * Sets the window to be floating.
         * @param floating Weather or not to be floating.
         */
        void SetFloating(bool floating);

        /**
         * Gets weather the window is fullscreen or not.
         * @return Fullscreen or windowed.
         */
        bool IsFullscreen() const { return m_fullscreen; }

        /**
         * Gets if the window is closed.
         * @return If the window is closed.
         */
        bool IsClosed() const { return m_closed; }

        /**
         * Gets if the window is selected.
         * @return If the window is selected.
         */
        bool IsFocused() const { return m_focused; }

        /**
         * Gets the windows is minimized.
         * @return If the window is minimized.
         */
        bool IsIconified() const { return m_iconified; }

        /**
         * Sets the window to be iconified (minimized).
         * @param iconify If the window will be set as iconified.
         */
        void SetIconified(bool iconify);

        /**
         * Gets the contents of the clipboard as a string.
         * @return If the clipboard contents.
         */
        std::string GetClipboard() const;

        /**
         * Sets the clipboard to the specified string.
         * @param string The string to set as the clipboard.
         */
        void SetClipboard(const std::string& string) const;

        /**
         * Gets if the display is selected.
         * @return If the display is selected.
         */
        bool IsWindowSelected() const { return m_glfw_window_selected; }

        /**
         * If the cursor is hidden, the mouse is the display locked if true.
         * @return If the cursor is hidden.
         */
        bool IsCursorHidden() const { return m_cursor_hidden; }

        /**
         * Sets if the operating systems cursor is hidden whilst in the display.
         * @param hidden If the system cursor should be hidden when not shown.
         */
        void SetCursorHidden(bool hidden);

        /**
         * Gets the current state of a key.
         * @param key The key to get the state of.
         * @return The keys state.
         */
        InputAction GetKeyAction(KeyCode key) const;

        /**
         * Gets the current state of a mouse button.
         * @param mouseButton The mouse button to get the state of.
         * @return The mouse buttons state.
         */
        InputAction GetMouseButtonAction(MouseButtonCode mouseButton) const;

        /**
         * Gets the mouses position.
         * @return The mouses position.
         */
        const glm::vec2& GetMousePosition() const { return m_mouse_position; }

        /**
         * Sets the mouse position.
         * @param position The new mouse position.
         */
        void SetMousePosition(const glm::vec2& m_mouse_position);

        /**
         * Gets the mouse position delta.
         * @return The mouse position delta.
         */
        const glm::vec2& GetMousePositionDelta() const { return m_mouse_position_delta; }

        /**
         * Gets the mouses virtual scroll position.
         * @return The mouses virtual scroll position.
         */
        const glm::vec2& GetMouseScroll() const { return m_mouse_scroll; }

        /**
         * Sets the mouse virtual scroll position.
         * @param scroll The new mouse virtual scroll position.
         */
        void SetMouseScroll(const glm::vec2& scroll);

        /**
         * Gets the mouse scroll delta.
         * @return The mouse scroll delta.
         */
        const glm::vec2& GetMouseScrollDelta() const { return m_mouse_scroll_delta; }

        GLFWwindow* GetGLFWWindow() const { return m_glfw_window; }

        /**
         * Called when the window is resized.
         */
        Signal<glm::ivec2>& OnSize() { return m_on_size_signal; }

        /**
         * Called when the window is moved.
         */
        Signal<glm::ivec2>& OnPosition() { return m_on_position_signal; }

        /**
         * Called when the windows title changed.
         */
        Signal<std::string>& OnTitle() { return m_on_title_signal; }

        /**
         * Called when the window has toggled borderless on or off.
         */
        Signal<bool>& OnBorderless() { return m_on_borderless_signal; }

        /**
         * Called when the window has toggled resizable on or off.
         */
        Signal<bool>& OnResizable() { return m_on_resizable_signal; }

        /**
         * Called when the window has toggled floating on or off.
         */
        Signal<bool>& OnFloating() { return m_on_floating_signal; }

        /**
         * Called when the has gone fullscreen or windowed.
         */
        Signal<bool>& OnFullscreen() { return m_on_fullscreen_signal; }

        /**
         * Called when the window requests a close.
         */
        Signal<>& OnClose() { return m_on_close_signal; }

        /**
         * Called when the window is focused or unfocused.
         */
        Signal<bool>& OnFocus() { return m_on_focus_signal; }

        /**
         * Called when the window is minimized or maximized.
         */
        Signal<bool>& OnIconify() { return m_on_iconify_signal; }

        /**
         * Called when the mouse enters the window.
         * @return The delegate.
         */
        Signal<bool>& OnEnter() { return m_on_enter_signal; }

        /**
         * Called when a group of files/folders is dropped onto the window.
         * @return The delegate.
         */
        Signal<std::vector<std::string>>& OnDrop() { return m_on_drop_signal; }

        /**
         * Called when a key changes state.
         * @return The delegate.
         */
        Signal<KeyCode, InputAction, uint8_t>& OnKey() { return m_on_key_signal; }

        /**
         * Called when a character has been typed.
         * @return The delegate.
         */
        Signal<char>& OnChar() { return m_on_char_signal; }

        /**
         * Called when a mouse button changes state.
         * @return The delegate.
         */
        Signal<MouseButtonCode, InputAction, uint8_t>& OnMouseButton() { return m_on_mouse_button_signal; }

        /**
         * Called when the mouse moves.
         * @return The delegate.
         */
        Signal<glm::vec2>& OnMousePosition() { return m_on_mouse_position_signal; }

        /**
         * Called when the scroll wheel changes.
         * @return The delegate.
         */
        Signal<glm::vec2>& OnMouseScroll() { return m_on_mouse_scroll_signal; }

    protected:
        friend void CallbackWindowPosition(GLFWwindow* glfwWindow, int32_t xpos, int32_t ypos);
        friend void CallbackWindowSize(GLFWwindow* glfwWindow, int32_t width, int32_t height);
        friend void CallbackWindowClose(GLFWwindow* glfwWindow);
        friend void CallbackWindowFocus(GLFWwindow* glfwWindow, int32_t focused);
        friend void CallbackWindowIconify(GLFWwindow* glfwWindow, int32_t iconified);
        friend void CallbackFramebufferSize(GLFWwindow* glfwWindow, int32_t width, int32_t height);
        friend void CallbackCursorEnter(GLFWwindow* glfwWindow, int32_t entered);
        friend void CallbackDrop(GLFWwindow* glfwWindow, int32_t count, const char** paths);
        friend void CallbackKey(GLFWwindow* glfwWindow, int32_t key, int32_t scancode, int32_t action, int32_t mods);
        friend void CallbackChar(GLFWwindow* glfwWindow, uint32_t codepoint);
        friend void CallbackMouseButton(GLFWwindow* glfwWindow, int32_t button, int32_t action, int32_t mods);
        friend void CallbackCursorPos(GLFWwindow* glfwWindow, double xpos, double ypos);
        friend void CallbackScroll(GLFWwindow* glfwWindow, double xoffset, double yoffset);

        static double SmoothScrollWheel(double value, float delta);

        std::size_t m_glfw_window_id;
        GLFWwindow* m_glfw_window = nullptr;

        glm::ivec2 m_size;
        glm::ivec2 m_fullscreen_size;

        glm::ivec2 m_position;

        std::string m_title;
        bool        m_borderless = false;
        bool        m_resizable  = false;
        bool        m_floating   = false;
        bool        m_fullscreen = false;

        bool m_closed    = false;
        bool m_focused   = false;
        bool m_iconified = false;

        bool m_glfw_window_selected = false;
        bool m_cursor_hidden        = false;

        glm::vec2 m_mouse_last_position;
        glm::vec2 m_mouse_position;
        glm::vec2 m_mouse_position_delta;
        glm::vec2 m_mouse_last_scroll;
        glm::vec2 m_mouse_scroll;
        glm::vec2 m_mouse_scroll_delta;

        Signal<glm::ivec2>                            m_on_size_signal;
        Signal<glm::ivec2>                            m_on_position_signal;
        Signal<std::string>                           m_on_title_signal;
        Signal<bool>                                  m_on_borderless_signal;
        Signal<bool>                                  m_on_resizable_signal;
        Signal<bool>                                  m_on_floating_signal;
        Signal<bool>                                  m_on_fullscreen_signal;
        Signal<>                                      m_on_close_signal;
        Signal<bool>                                  m_on_focus_signal;
        Signal<bool>                                  m_on_iconify_signal;
        Signal<bool>                                  m_on_enter_signal;
        Signal<std::vector<std::string>>              m_on_drop_signal;
        Signal<KeyCode, InputAction, uint8_t>         m_on_key_signal;
        Signal<char>                                  m_on_char_signal;
        Signal<MouseButtonCode, InputAction, uint8_t> m_on_mouse_button_signal;
        Signal<glm::vec2>                             m_on_mouse_position_signal;
        Signal<glm::vec2>                             m_on_mouse_scroll_signal;
    };
} // namespace Meow