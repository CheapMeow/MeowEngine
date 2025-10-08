#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    struct PerFrameData
    {
        // graphics

        vk::raii::CommandBuffer graphics_command_buffer  = nullptr;
        vk::raii::Fence         graphics_in_flight_fence = nullptr;

        // compute

        vk::raii::CommandBuffer compute_command_buffer     = nullptr;
        vk::raii::Semaphore     compute_finished_semaphore = nullptr;
        vk::raii::Fence         compute_in_flight_fence    = nullptr;

        PerFrameData() {}

        PerFrameData(std::nullptr_t) {}
    };

    struct PerSwapChainImageData
    {
        vk::raii::Semaphore present_finished_semaphore = nullptr;
        vk::raii::Semaphore render_finished_semaphore  = nullptr;

        PerSwapChainImageData() {}

        PerSwapChainImageData(std::nullptr_t) {}
    };
} // namespace Meow
