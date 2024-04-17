#pragma once

#include "core/base/non_copyable.h"

#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    class DeferredPass : NonCopyable
    {
    public:
        DeferredPass(std::nullptr_t) {}

        DeferredPass(vk::raii::Device const& device,
                     vk::Format              color_format,
                     vk::Format              depth_format,
                     vk::AttachmentLoadOp    load_op            = vk::AttachmentLoadOp::eClear,
                     vk::ImageLayout         color_final_layout = vk::ImageLayout::ePresentSrcKHR);

        void RefreshFrameBuffers(vk::raii::Device const&                 device,
                                 std::vector<vk::raii::ImageView> const& image_views,
                                 vk::raii::ImageView const*              p_depth_image_view,
                                 vk::Extent2D const&                     extent);

        vk::raii::RenderPass               render_pass = nullptr;
        std::vector<vk::raii::Framebuffer> framebuffers;
    };
} // namespace Meow
