#include "swapchain_data.h"

namespace Meow
{
    SwapChainData::SwapChainData(vk::raii::PhysicalDevice const& physical_device,
                                 vk::raii::Device const&         device,
                                 vk::raii::SurfaceKHR const&     surface,
                                 vk::Extent2D const&             extent,
                                 vk::ImageUsageFlags             usage,
                                 vk::raii::SwapchainKHR const*   p_old_swapchain,
                                 uint32_t                        graphics_queue_family_index,
                                 uint32_t                        present_queue_family_index)
    {
        vk::SurfaceFormatKHR surface_format = PickSurfaceFormat(physical_device.getSurfaceFormatsKHR(*surface));
        color_format                        = surface_format.format;

        vk::SurfaceCapabilitiesKHR surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(*surface);
        vk::Extent2D               swapchain_extent;
        if (surface_capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
        {
            // If the surface size is undefined, the size is set to the size of the images requested.
            swapchain_extent.width = glm::clamp(
                extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
            swapchain_extent.height = glm::clamp(
                extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
        }
        else
        {
            // If the surface size is defined, the swap chain size must match
            swapchain_extent = surface_capabilities.currentExtent;
        }
        vk::SurfaceTransformFlagBitsKHR pre_transform =
            (surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ?
                vk::SurfaceTransformFlagBitsKHR::eIdentity :
                surface_capabilities.currentTransform;
        vk::CompositeAlphaFlagBitsKHR composite_alpha =
            (surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ?
                vk::CompositeAlphaFlagBitsKHR::ePreMultiplied :
            (surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ?
                vk::CompositeAlphaFlagBitsKHR::ePostMultiplied :
            (surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ?
                vk::CompositeAlphaFlagBitsKHR::eInherit :
                vk::CompositeAlphaFlagBitsKHR::eOpaque;
        vk::PresentModeKHR         present_mode = PickPresentMode(physical_device.getSurfacePresentModesKHR(*surface));
        vk::SwapchainCreateInfoKHR swap_chain_create_info(
            {},
            *surface,
            glm::clamp(3u, surface_capabilities.minImageCount, surface_capabilities.maxImageCount),
            color_format,
            surface_format.colorSpace,
            swapchain_extent,
            1,
            usage,
            vk::SharingMode::eExclusive,
            {},
            pre_transform,
            composite_alpha,
            present_mode,
            true,
            p_old_swapchain ? **p_old_swapchain : nullptr);
        if (graphics_queue_family_index != present_queue_family_index)
        {
            uint32_t queueFamilyIndices[2] = {graphics_queue_family_index, present_queue_family_index};
            // If the graphics and present queues are from different queue families, we either have to
            // explicitly transfer ownership of images between the queues, or we have to create the swapchain
            // with imageSharingMode as vk::SharingMode::eConcurrent
            swap_chain_create_info.imageSharingMode      = vk::SharingMode::eConcurrent;
            swap_chain_create_info.queueFamilyIndexCount = 2;
            swap_chain_create_info.pQueueFamilyIndices   = queueFamilyIndices;
        }
        swap_chain = vk::raii::SwapchainKHR(device, swap_chain_create_info);

        images = swap_chain.getImages();

        image_views.reserve(images.size());
        vk::ImageViewCreateInfo image_view_create_info(
            {}, {}, vk::ImageViewType::e2D, color_format, {}, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        for (auto image : images)
        {
            image_view_create_info.image = image;
            image_views.emplace_back(device, image_view_create_info);
        }
    }
} // namespace Meow