#pragma once

#include "core/base/macro.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    struct LIBRARY_API SurfaceData
    {
        vk::Extent2D         extent;
        vk::raii::SurfaceKHR surface = nullptr;

        SurfaceData(vk::raii::Instance const& instance, GLFWwindow* glfw_window, vk::Extent2D const& extent_);
        SurfaceData(std::nullptr_t) {}
    };
} // namespace Meow
