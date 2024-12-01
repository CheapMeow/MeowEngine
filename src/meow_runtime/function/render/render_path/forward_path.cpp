#include "forward_path.h"

#include "function/global/runtime_context.h"

namespace Meow
{
    void ForwardPath::RefreshAttachments(vk::Format color_format, const vk::Extent2D& extent)
    {
        const vk::raii::Device&      logical_device = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool& onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        // clear

        m_depth_attachment = nullptr;

        // Create attachment

        m_depth_attachment = ImageData::CreateAttachment(m_depth_format,
                                                         extent,
                                                         vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                         vk::ImageAspectFlagBits::eDepth,
                                                         {},
                                                         false);

        OneTimeSubmit(logical_device,
                      onetime_submit_command_pool,
                      graphics_queue,
                      [&](vk::raii::CommandBuffer const& command_buffer) {
                          TransitLayout(command_buffer,
                                        *m_depth_attachment.image,
                                        vk::ImageLayout::eUndefined,
                                        vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                        {m_depth_attachment.aspect_mask, 0, 1, 0, 1});
                      });

#ifdef MEOW_EDITOR
        vk::Extent2D temp_extent  = {extent.width / 2, extent.height / 2};
        m_offscreen_render_target = ImageData::CreateRenderTarget(color_format,
                                                                  temp_extent,
                                                                  vk::ImageUsageFlagBits::eColorAttachment |
                                                                      vk::ImageUsageFlagBits::eInputAttachment,
                                                                  vk::ImageAspectFlagBits::eColor,
                                                                  {},
                                                                  false);

        m_imgui_pass.RefreshOffscreenRenderTarget(*m_offscreen_render_target.sampler,
                                                  *m_offscreen_render_target.image_view,
                                                  static_cast<VkImageLayout>(m_offscreen_render_target.layout));
#endif
    }

    void ForwardPath::UpdateUniformBuffer() { m_forward_light_pass.UpdateUniformBuffer(); }

    void ForwardPath::Draw(const vk::raii::CommandBuffer& command_buffer,
                           vk::Extent2D                   extent,
                           const vk::Image&               swapchain_image,
                           const vk::raii::ImageView&     swapchain_image_view)
    {
        FUNCTION_TIMER();

        {
#ifndef MEOW_EDITOR
            TransitLayout(command_buffer,
                          swapchain_image,
                          vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eColorAttachmentOptimal,
                          {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
#endif

            vk::RenderingAttachmentInfoKHR render_target_info(
#ifdef MEOW_EDITOR
                *m_offscreen_render_target.image_view,    /* imageView */
                vk::ImageLayout::eColorAttachmentOptimal, /* imageLayout */
#else
                *swapchain_image_view,                    /* imageView */
                vk::ImageLayout::eColorAttachmentOptimal, /* imageLayout */
#endif
                vk::ResolveModeFlagBits::eNone,               /* resolveMode */
                {},                                           /* resolveImageView */
                {},                                           /* resolveImageLayout */
                vk::AttachmentLoadOp::eClear,                 /* loadOp */
                vk::AttachmentStoreOp::eStore,                /* storeOp */
                vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f)); /* clearValue */

            vk::RenderingAttachmentInfoKHR depth_attachment_info(
                *m_depth_attachment.image_view,                  /* imageView */
                vk::ImageLayout::eDepthStencilAttachmentOptimal, /* imageLayout */
                vk::ResolveModeFlagBits::eNone,                  /* resolveMode */
                {},                                              /* resolveImageView */
                {},                                              /* resolveImageLayout */
                vk::AttachmentLoadOp::eClear,                    /* loadOp */
                vk::AttachmentStoreOp::eStore,                   /* storeOp */
                vk::ClearDepthStencilValue(1.0f, 0));            /* clearValue */

            vk::RenderingInfoKHR rendering_info({},                                 /* flags */
                                                vk::Rect2D(vk::Offset2D(), extent), /* renderArea */
                                                1,                                  /* layerCount */
                                                0,                                  /* viewMask */
                                                1,                                  /* colorAttachmentCount */
                                                &render_target_info,                /* pColorAttachments */
                                                &depth_attachment_info);            /* pDepthAttachment */

            command_buffer.beginRenderingKHR(rendering_info);

            m_forward_light_pass.Draw(command_buffer);

            command_buffer.endRenderingKHR();

#ifndef MEOW_EDITOR
            TransitLayout(command_buffer,
                          swapchain_image,
                          vk::ImageLayout::eColorAttachmentOptimal,
                          vk::ImageLayout::ePresentSrcKHR,
                          {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
#endif
        }

#ifdef MEOW_EDITOR
        {
            TransitLayout(command_buffer,
                          *m_offscreen_render_target.image,
                          vk::ImageLayout::eColorAttachmentOptimal,
                          vk::ImageLayout::eShaderReadOnlyOptimal,
                          {m_offscreen_render_target.aspect_mask, 0, 1, 0, 1});

            TransitLayout(command_buffer,
                          swapchain_image,
                          vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eColorAttachmentOptimal,
                          {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

            m_imgui_pass.DrawImGui();

            vk::RenderingAttachmentInfoKHR render_target_info(
                *swapchain_image_view,                        /* imageView */
                vk::ImageLayout::eColorAttachmentOptimal,     /* imageLayout */
                vk::ResolveModeFlagBits::eNone,               /* resolveMode */
                {},                                           /* resolveImageView */
                {},                                           /* resolveImageLayout */
                vk::AttachmentLoadOp::eClear,                 /* loadOp */
                vk::AttachmentStoreOp::eStore,                /* storeOp */
                vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f)); /* clearValue */

            vk::RenderingInfoKHR rendering_info({},                                 /* flags */
                                                vk::Rect2D(vk::Offset2D(), extent), /* renderArea */
                                                1,                                  /* layerCount */
                                                0,                                  /* viewMask */
                                                1,                                  /* colorAttachmentCount */
                                                &render_target_info,                /* pColorAttachments */
                                                nullptr);                           /* pDepthAttachment */

            command_buffer.beginRenderingKHR(rendering_info);
            m_imgui_pass.Draw(command_buffer);
            command_buffer.endRenderingKHR();

            TransitLayout(command_buffer,
                          swapchain_image,
                          vk::ImageLayout::eColorAttachmentOptimal,
                          vk::ImageLayout::ePresentSrcKHR,
                          {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        }
#endif
    }

    void ForwardPath::AfterPresent()
    {
        FUNCTION_TIMER();

        m_forward_light_pass.AfterPresent();
    }

    void swap(ForwardPath& lhs, ForwardPath& rhs)
    {
        using std::swap;

        swap(static_cast<RenderPath&>(lhs), static_cast<RenderPath&>(rhs));

        swap(lhs.m_forward_light_pass, rhs.m_forward_light_pass);
    }
} // namespace Meow
