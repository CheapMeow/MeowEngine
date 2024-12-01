#pragma once

#include "function/render/utils/vulkan_initialize_utils.hpp"

namespace Meow
{
    struct SwapChainData
    {
        vk::Format                       color_format;
        vk::raii::SwapchainKHR           swap_chain = nullptr;
        std::vector<vk::Image>           images;
        std::vector<vk::raii::ImageView> image_views;

        SwapChainData(vk::raii::PhysicalDevice const& physical_device,
                      vk::raii::Device const&         device,
                      vk::raii::SurfaceKHR const&     surface,
                      vk::Extent2D const&             extent,
                      vk::ImageUsageFlags             usage,
                      vk::raii::SwapchainKHR const*   p_old_swapchain,
                      uint32_t                        graphics_queue_family_index,
                      uint32_t                        present_queue_family_index);

        SwapChainData(std::nullptr_t) {}
    };

} // namespace Meow
