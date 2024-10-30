#include "surface_data.h"

namespace Meow
{
    SurfaceData::SurfaceData(vk::raii::Instance const& instance, GLFWwindow* glfw_window, vk::Extent2D const& extent_)
        : extent(extent_)
    {
        VkSurfaceKHR _surface;
        VkResult     err = glfwCreateWindowSurface(static_cast<VkInstance>(*instance), glfw_window, nullptr, &_surface);
        if (err != VK_SUCCESS)
            throw std::runtime_error("Failed to create window!");
        surface = vk::raii::SurfaceKHR(instance, _surface);
    }
} // namespace Meow
