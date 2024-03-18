#pragma once

#include "function/render/structs/image_data.h"

namespace Meow
{
    struct DepthBufferData : public ImageData
    {
        DepthBufferData(vk::raii::PhysicalDevice const& physical_device,
                        vk::raii::Device const&         device,
                        vk::Format                      format,
                        vk::Extent2D const&             extent)
            : ImageData(physical_device,
                        device,
                        format,
                        extent,
                        vk::ImageTiling::eOptimal,
                        vk::ImageUsageFlagBits::eDepthStencilAttachment,
                        vk::ImageLayout::eUndefined,
                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                        vk::ImageAspectFlagBits::eDepth)
        {}

        DepthBufferData(std::nullptr_t)
            : ImageData(nullptr)
        {}
    };
} // namespace Meow
