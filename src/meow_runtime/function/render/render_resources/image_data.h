#pragma once

#include "buffer_data.h"
#include "function/render/utils/vulkan_utils.hpp"
#include "render_resource_base.h"

namespace Meow
{
    struct ImageData : public RenderResourceBase
    {
    public:
        vk::Format   format;
        vk::Extent2D extent;
        uint32_t     size;
        /**
         * @brief `ImageAspectFlags` should be stored because it is only known in specific ctor, and it can't be
         * deducted from other information in other places. For example, in `SetLayout`, if you
         * haven't store `ImageAspectFlags`, then you should deduct ImageAspectFlags from new_image_layout, if
         * `new_image_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal` then you set
         * `vk::ImageAspectFlagBits::eDepth`, otherwise you set `vk::ImageAspectFlagBits::eColor`. But
         *
         */
        vk::ImageAspectFlags aspect_mask;

        /**
         * @brief The `vk::raii::DeviceMemory` should be destroyed before the `vk::raii::Image` it is bound to; to get
         * that order with the standard destructor of the `ImageData`, the order of `vk::raii::DeviceMemory` and
         * `vk::raii::Image` here matters
         */
        vk::raii::DeviceMemory device_memory = nullptr;
        vk::raii::Image        image         = nullptr;
        vk::raii::ImageView    image_view    = nullptr;
        vk::raii::Sampler      sampler       = nullptr;
        bool                   need_staging;
        BufferData             staging_buffer_data = nullptr;
        vk::ImageLayout        layout;

        ImageData(std::nullptr_t) {}

        static ImageData CreateTexture(const std::string&     file_path,
                                       vk::Format             format               = vk::Format::eR8G8B8A8Unorm,
                                       vk::ImageUsageFlags    usage_flags          = {},
                                       vk::ImageAspectFlags   aspect_mask          = vk::ImageAspectFlagBits::eColor,
                                       vk::FormatFeatureFlags format_feature_flags = {},
                                       bool                   anisotropy_enable    = false,
                                       bool                   force_staging        = true);

        static ImageData CreateAttachment(vk::Format             format               = vk::Format::eR8G8B8A8Unorm,
                                          const vk::Extent2D&    extent               = {256, 256},
                                          vk::ImageUsageFlags    usage_flags          = {},
                                          vk::ImageAspectFlags   aspect_mask          = vk::ImageAspectFlagBits::eColor,
                                          vk::FormatFeatureFlags format_feature_flags = {},
                                          bool                   anisotropy_enable    = false);

        static ImageData CreateRenderTarget(vk::Format             format      = vk::Format::eR8G8B8A8Unorm,
                                            const vk::Extent2D&    extent      = {256, 256},
                                            vk::ImageUsageFlags    usage_flags = {},
                                            vk::ImageAspectFlags   aspect_mask = vk::ImageAspectFlagBits::eColor,
                                            vk::FormatFeatureFlags format_feature_flags = {},
                                            bool                   anisotropy_enable    = false);

        static ImageData CreateCubemap(const std::vector<std::string>& file_paths,
                                       vk::Format                      format      = vk::Format::eR32G32B32A32Sfloat,
                                       vk::ImageUsageFlags             usage_flags = {},
                                       vk::ImageAspectFlags            aspect_mask = vk::ImageAspectFlagBits::eColor,
                                       vk::FormatFeatureFlags          format_feature_flags = {},
                                       bool                            anisotropy_enable    = false,
                                       bool                            force_staging        = true);
    };
} // namespace Meow
