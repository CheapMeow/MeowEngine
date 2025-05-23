#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    struct PerFrameData
    {
        vk::raii::CommandPool command_pool = nullptr;

        // graphics

        vk::raii::CommandBuffer command_buffer            = nullptr;
        vk::raii::Semaphore     image_acquired_semaphore  = nullptr;
        vk::raii::Semaphore     render_finished_semaphore = nullptr;
        vk::raii::Fence         in_flight_fence           = nullptr;

        // compute

        vk::raii::CommandBuffer compute_command_buffer     = nullptr;
        vk::raii::Semaphore     compute_finished_semaphore = nullptr;
        vk::raii::Fence         compute_in_flight_fence    = nullptr;

        PerFrameData() {}

        PerFrameData(std::nullptr_t) {}
    };
} // namespace Meow
