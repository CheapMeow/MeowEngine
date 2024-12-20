#pragma once

#include "buffer_data.h"
#include "function/render/utils/vulkan_initialize_utils.hpp"
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

        /**
         * @brief Set the Image Layout.
         *
         * If an image is not initialized in any specific layout, so we need to do a layout transition.
         *
         * Such as you need the driver puts the texture into Linear layout,
         * which is the best for copying data from a buffer into a texture.
         *
         * To perform layout transitions, we need to use pipeline barriers. Pipeline barriers can control how the GPU
         * overlaps commands before and after the barrier, but if you do pipeline barriers with image barriers, the
         * driver can also transform the image to the correct formats and layouts.
         *
         * @param command_buffer Usually use command buffer in upload context
         * @param old_image_layout Old image layout
         * @param new_image_layout New image layout
         */
        void TransitLayout(const vk::raii::CommandBuffer& command_buffer,
                           vk::ImageLayout                old_image_layout,
                           vk::ImageLayout                new_image_layout,
                           vk::ImageSubresourceRange      image_subresource_range);

        static std::shared_ptr<ImageData>
        CreateTexture(const std::string&     file_path,
                      vk::Format             format               = vk::Format::eR8G8B8A8Unorm,
                      vk::ImageUsageFlags    usage_flags          = {},
                      vk::ImageAspectFlags   aspect_mask          = vk::ImageAspectFlagBits::eColor,
                      vk::FormatFeatureFlags format_feature_flags = {},
                      bool                   anisotropy_enable    = false,
                      bool                   force_staging        = true);

        static std::shared_ptr<ImageData>
        CreateAttachment(vk::Format             format               = vk::Format::eR8G8B8A8Unorm,
                         const vk::Extent2D&    extent               = {256, 256},
                         vk::ImageUsageFlags    usage_flags          = {},
                         vk::ImageAspectFlags   aspect_mask          = vk::ImageAspectFlagBits::eColor,
                         vk::FormatFeatureFlags format_feature_flags = {},
                         bool                   anisotropy_enable    = false);

        static std::shared_ptr<ImageData>
        CreateRenderTarget(vk::Format             format               = vk::Format::eR8G8B8A8Unorm,
                           const vk::Extent2D&    extent               = {256, 256},
                           vk::ImageUsageFlags    usage_flags          = {},
                           vk::ImageAspectFlags   aspect_mask          = vk::ImageAspectFlagBits::eColor,
                           vk::FormatFeatureFlags format_feature_flags = {},
                           bool                   anisotropy_enable    = false);

        static std::shared_ptr<ImageData>
        CreateCubemap(const std::vector<std::string>& file_paths,
                      vk::Format                      format               = vk::Format::eR32G32B32A32Sfloat,
                      vk::ImageUsageFlags             usage_flags          = {},
                      vk::ImageAspectFlags            aspect_mask          = vk::ImageAspectFlagBits::eColor,
                      vk::FormatFeatureFlags          format_feature_flags = {},
                      bool                            anisotropy_enable    = false,
                      bool                            force_staging        = true);
    };
} // namespace Meow
