#include "deferred_path.h"

#include "function/global/runtime_context.h"

namespace Meow
{
    void DeferredPath::RefreshAttachments(vk::Format color_format, const vk::Extent2D& extent)
    {
        const vk::raii::Device&      logical_device = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool& onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();
        // clear

        m_color_attachment    = nullptr;
        m_normal_attachment   = nullptr;
        m_position_attachment = nullptr;
        m_depth_attachment    = nullptr;

        // Create attachment

        vk::Extent2D temp_extent = {extent.width / 2, extent.height / 2};

        m_color_attachment = ImageData::CreateAttachment(color_format,
                                                         temp_extent,
                                                         vk::ImageUsageFlagBits::eColorAttachment |
                                                             vk::ImageUsageFlagBits::eInputAttachment,
                                                         vk::ImageAspectFlagBits::eColor,
                                                         {},
                                                         false);

        m_normal_attachment = ImageData::CreateAttachment(vk::Format::eB8G8R8A8Unorm,
                                                          temp_extent,
                                                          vk::ImageUsageFlagBits::eColorAttachment |
                                                              vk::ImageUsageFlagBits::eInputAttachment,
                                                          vk::ImageAspectFlagBits::eColor,
                                                          {},
                                                          false);

        m_position_attachment = ImageData::CreateAttachment(vk::Format::eR16G16B16A16Sfloat,
                                                            temp_extent,
                                                            vk::ImageUsageFlagBits::eColorAttachment |
                                                                vk::ImageUsageFlagBits::eInputAttachment,
                                                            vk::ImageAspectFlagBits::eColor,
                                                            {},
                                                            false);

        m_depth_attachment = ImageData::CreateAttachment(m_depth_format,
                                                         temp_extent,
                                                         vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                         vk::ImageAspectFlagBits::eDepth,
                                                         {},
                                                         false);

        OneTimeSubmit(logical_device,
                      onetime_submit_command_pool,
                      graphics_queue,
                      [&](vk::raii::CommandBuffer const& command_buffer) {
                          TransitLayout(command_buffer,
                                        *m_color_attachment.image,
                                        vk::ImageLayout::eUndefined,
                                        vk::ImageLayout::eColorAttachmentOptimal,
                                        {m_color_attachment.aspect_mask, 0, 1, 0, 1});

                          TransitLayout(command_buffer,
                                        *m_normal_attachment.image,
                                        vk::ImageLayout::eUndefined,
                                        vk::ImageLayout::eColorAttachmentOptimal,
                                        {m_normal_attachment.aspect_mask, 0, 1, 0, 1});

                          TransitLayout(command_buffer,
                                        *m_position_attachment.image,
                                        vk::ImageLayout::eUndefined,
                                        vk::ImageLayout::eColorAttachmentOptimal,
                                        {m_position_attachment.aspect_mask, 0, 1, 0, 1});

                          TransitLayout(command_buffer,
                                        *m_depth_attachment.image,
                                        vk::ImageLayout::eUndefined,
                                        vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                        {m_depth_attachment.aspect_mask, 0, 1, 0, 1});
                      });

        m_deferred_light_pass.RefreshAttachments(
            m_color_attachment, m_normal_attachment, m_position_attachment, m_depth_attachment);

#ifdef MEOW_EDITOR
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

    void DeferredPath::UpdateUniformBuffer()
    {
        m_gbuffer_pass.UpdateUniformBuffer();
        m_deferred_light_pass.UpdateUniformBuffer();
    }

    void DeferredPath::Draw(const vk::raii::CommandBuffer& command_buffer,
                            vk::Extent2D                   extent,
                            const vk::Image&               swapchain_image,
                            const vk::raii::ImageView&     swapchain_image_view)
    {
        FUNCTION_TIMER();

        {
#ifdef MEOW_EDITOR
            TransitLayout(command_buffer,
                          *m_offscreen_render_target.image,
                          vk::ImageLayout::eShaderReadOnlyOptimal,
                          vk::ImageLayout::eColorAttachmentOptimal,
                          {m_offscreen_render_target.aspect_mask, 0, 1, 0, 1});
#else
            TransitLayout(command_buffer,
                          swapchain_image,
                          vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eColorAttachmentOptimal,
                          {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
#endif

            vk::RenderingAttachmentInfoKHR color_attachment_infos[4] = {
                {
#ifdef MEOW_EDITOR
                    *m_offscreen_render_target.image_view,    /* imageView */
                    vk::ImageLayout::eColorAttachmentOptimal, /* imageLayout */
#else
                    *swapchain_image_view,                    /* imageView */
                    vk::ImageLayout::eColorAttachmentOptimal, /* imageLayout */
#endif
                    vk::ResolveModeFlagBits::eNone,             /* resolveMode */
                    {},                                         /* resolveImageView */
                    {},                                         /* resolveImageLayout */
                    vk::AttachmentLoadOp::eClear,               /* loadOp */
                    vk::AttachmentStoreOp::eStore,              /* storeOp */
                    vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f) /* clearValue */
                },
                {
                    *m_color_attachment.image_view,             /* imageView */
                    vk::ImageLayout::eColorAttachmentOptimal,   /* imageLayout */
                    vk::ResolveModeFlagBits::eNone,             /* resolveMode */
                    {},                                         /* resolveImageView */
                    {},                                         /* resolveImageLayout */
                    vk::AttachmentLoadOp::eClear,               /* loadOp */
                    vk::AttachmentStoreOp::eStore,              /* storeOp */
                    vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f) /* clearValue */
                },
                {
                    *m_normal_attachment.image_view,            /* imageView */
                    vk::ImageLayout::eColorAttachmentOptimal,   /* imageLayout */
                    vk::ResolveModeFlagBits::eNone,             /* resolveMode */
                    {},                                         /* resolveImageView */
                    {},                                         /* resolveImageLayout */
                    vk::AttachmentLoadOp::eClear,               /* loadOp */
                    vk::AttachmentStoreOp::eStore,              /* storeOp */
                    vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f) /* clearValue */
                },
                {
                    *m_position_attachment.image_view,          /* imageView */
                    vk::ImageLayout::eColorAttachmentOptimal,   /* imageLayout */
                    vk::ResolveModeFlagBits::eNone,             /* resolveMode */
                    {},                                         /* resolveImageView */
                    {},                                         /* resolveImageLayout */
                    vk::AttachmentLoadOp::eClear,               /* loadOp */
                    vk::AttachmentStoreOp::eStore,              /* storeOp */
                    vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f) /* clearValue */
                },
            };

            vk::RenderingAttachmentInfoKHR depth_attachment_info(
                *m_depth_attachment.image_view,                  /* imageView */
                vk::ImageLayout::eDepthStencilAttachmentOptimal, /* imageLayout */
                vk::ResolveModeFlagBits::eNone,                  /* resolveMode */
                {},                                              /* resolveImageView */
                {},                                              /* resolveImageLayout */
                vk::AttachmentLoadOp::eClear,                    /* loadOp */
                vk::AttachmentStoreOp::eStore,                   /* storeOp */
                vk::ClearDepthStencilValue(1.0f, 0));            /* clearValue */

            vk::Extent2D temp_extent = {extent.width / 2, extent.height / 2};

            vk::RenderingInfoKHR rendering_info({},                                      /* flags */
                                                vk::Rect2D(vk::Offset2D(), temp_extent), /* renderArea */
                                                1,                                       /* layerCount */
                                                0,                                       /* viewMask */
                                                4,                                       /* colorAttachmentCount */
                                                color_attachment_infos,                  /* pColorAttachments */
                                                &depth_attachment_info);                 /* pDepthAttachment */

            command_buffer.beginRenderingKHR(rendering_info);

            m_gbuffer_pass.Draw(command_buffer);

            vk::MemoryBarrier2 memory_barrier(vk::PipelineStageFlagBits2::eColorAttachmentOutput, /* srcStageMask */
                                              vk::AccessFlagBits2::eColorAttachmentWrite,         /* srcAccessMask */
                                              vk::PipelineStageFlagBits2::eFragmentShader,        /* dstStageMask */
                                              vk::AccessFlagBits2::eColorAttachmentRead);         /* dstAccessMask */

            vk::DependencyInfo dependency_info(vk::DependencyFlagBits::eByRegion, /* dependencyFlags */
                                               1,                                 /* memoryBarrierCount */
                                               &memory_barrier);                  /* pMemoryBarriers */

            command_buffer.pipelineBarrier2KHR(dependency_info);

            m_deferred_light_pass.Draw(command_buffer);

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

    void DeferredPath::AfterPresent()
    {
        FUNCTION_TIMER();

        m_gbuffer_pass.AfterPresent();
        m_deferred_light_pass.AfterPresent();
    }

    void swap(DeferredPath& lhs, DeferredPath& rhs)
    {
        using std::swap;

        swap(static_cast<RenderPath&>(lhs), static_cast<RenderPath&>(rhs));

        swap(lhs.m_gbuffer_pass, rhs.m_gbuffer_pass);
        swap(lhs.m_deferred_light_pass, rhs.m_deferred_light_pass);

        swap(lhs.m_color_attachment, rhs.m_color_attachment);
        swap(lhs.m_normal_attachment, rhs.m_normal_attachment);
        swap(lhs.m_position_attachment, rhs.m_position_attachment);
    }
} // namespace Meow
