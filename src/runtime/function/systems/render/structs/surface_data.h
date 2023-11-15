#pragma once

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    struct SurfaceData
    {
        SurfaceData(vk::raii::Instance const& instance, GLFWwindow* glfw_window, vk::Extent2D const& extent_)
            : extent(extent_)
        {
            VkSurfaceKHR _surface;
            VkResult err = glfwCreateWindowSurface(static_cast<VkInstance>(*instance), glfw_window, nullptr, &_surface);
            if (err != VK_SUCCESS)
                throw std::runtime_error("Failed to create window!");
            surface = vk::raii::SurfaceKHR(instance, _surface);
        }

        SurfaceData(std::nullptr_t) {}

        vk::Extent2D         extent;
        vk::raii::SurfaceKHR surface = nullptr;
    };
} // namespace Meow
