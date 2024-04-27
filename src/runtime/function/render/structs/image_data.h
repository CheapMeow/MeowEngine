#pragma once

#include "core/base/non_copyable.h"
#include "function/render/utils/vulkan_initialize_utils.hpp"

namespace Meow
{
    struct ImageData : NonCopyable
    {
        // the DeviceMemory should be destroyed before the Image it is bound to; to get that order with the standard
        // destructor of the ImageData, the order of DeviceMemory and Image here matters
        vk::Format             format;
        vk::raii::DeviceMemory device_memory = nullptr;
        vk::raii::Image        image         = nullptr;
        vk::raii::ImageView    image_view    = nullptr;

        ImageData(vk::raii::PhysicalDevice const& physical_device,
                  vk::raii::Device const&         device,
                  vk::Format                      format_,
                  vk::Extent2D const&             extent,
                  vk::ImageTiling                 tiling,
                  vk::ImageUsageFlags             usage,
                  vk::ImageLayout                 initial_layout,
                  vk::MemoryPropertyFlags         memory_properties,
                  vk::ImageAspectFlags            aspect_mask)
            : format(format_)
            , image(device,
                    {vk::ImageCreateFlags(),
                     vk::ImageType::e2D,
                     format,
                     vk::Extent3D(extent, 1),
                     1,
                     1,
                     vk::SampleCountFlagBits::e1,
                     tiling,
                     usage | vk::ImageUsageFlagBits::eSampled,
                     vk::SharingMode::eExclusive,
                     {},
                     initial_layout})
        {
            device_memory = AllocateDeviceMemory(
                device, physical_device.getMemoryProperties(), image.getMemoryRequirements(), memory_properties);
            image.bindMemory(*device_memory, 0);
            image_view = vk::raii::ImageView(
                device,
                vk::ImageViewCreateInfo({}, *image, vk::ImageViewType::e2D, format, {}, {aspect_mask, 0, 1, 0, 1}));
        }

        ImageData(std::nullptr_t) {}
    };
} // namespace Meow
