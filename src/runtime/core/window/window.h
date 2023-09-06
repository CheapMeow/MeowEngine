#pragma once

#include "core/base/non_copyable.h"
#include "core/input/input_enum.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Meow
{
    using WindowId = std::size_t;

    /**
     * @brief Window controls glfw window and input.
     */
    class Window : NonCopyable
    {
    public:
        Window(std::size_t id);
        ~Window();

        void Update(double dt);

    private:
        GLFWwindow* m_Window = nullptr;
    };
} // namespace Meow