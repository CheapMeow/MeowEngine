#pragma once

#include <GLFW/glfw3.h>

namespace Meow
{
    namespace Time
    {
        double GetTime() { return glfwGetTime(); }
    } // namespace Time
} // namespace Meow