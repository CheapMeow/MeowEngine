#include "deferred_pass.h"

namespace Meow
{
    DeferredPass::DeferredPass(vk::raii::Device const& device,
                               vk::Format              color_format,
                               vk::Format              depth_format,
                               vk::AttachmentLoadOp    load_op,
                               vk::ImageLayout         color_final_layout)
    {
        std::vector<vk::AttachmentDescription> attachment_descriptions;
        assert(color_format != vk::Format::eUndefined);
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                                             color_format,
                                             vk::SampleCountFlagBits::e1,
                                             load_op,
                                             vk::AttachmentStoreOp::eStore,
                                             vk::AttachmentLoadOp::eDontCare,
                                             vk::AttachmentStoreOp::eDontCare,
                                             vk::ImageLayout::eUndefined,
                                             color_final_layout);
        if (depth_format != vk::Format::eUndefined)
        {
            attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                                                 depth_format,
                                                 vk::SampleCountFlagBits::e1,
                                                 load_op,
                                                 vk::AttachmentStoreOp::eDontCare,
                                                 vk::AttachmentLoadOp::eDontCare,
                                                 vk::AttachmentStoreOp::eDontCare,
                                                 vk::ImageLayout::eUndefined,
                                                 vk::ImageLayout::eDepthStencilAttachmentOptimal);
        }
        vk::AttachmentReference            color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal);
        vk::AttachmentReference            depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
        vk::SubpassDescription             subpass_description(vk::SubpassDescriptionFlags(),
                                                   vk::PipelineBindPoint::eGraphics,
                                                               {},
                                                   color_attachment,
                                                               {},
                                                   (depth_format != vk::Format::eUndefined) ? &depth_attachment :
                                                                                                          nullptr);
        std::vector<vk::SubpassDependency> dependencies;
        dependencies.emplace_back(VK_SUBPASS_EXTERNAL,
                                  0,
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                  vk::AccessFlagBits::eNone,
                                  vk::AccessFlagBits::eColorAttachmentWrite);
        vk::RenderPassCreateInfo render_pass_create_info(
            vk::RenderPassCreateFlags(), attachment_descriptions, subpass_description, dependencies);

        render_pass = vk::raii::RenderPass(device, render_pass_create_info);
    }

    void DeferredPass::RefreshFrameBuffers(vk::raii::Device const&                 device,
                                           std::vector<vk::raii::ImageView> const& image_views,
                                           vk::raii::ImageView const*              p_depth_image_view,
                                           vk::Extent2D const&                     extent)
    {
        vk::ImageView attachments[2];
        attachments[1] = p_depth_image_view ? **p_depth_image_view : vk::ImageView();

        vk::FramebufferCreateInfo framebuffer_create_info(vk::FramebufferCreateFlags(),
                                                          *render_pass,
                                                          p_depth_image_view ? 2 : 1,
                                                          attachments,
                                                          extent.width,
                                                          extent.height,
                                                          1);

        framebuffers.reserve(image_views.size());
        for (auto const& imageView : image_views)
        {
            attachments[0] = *imageView;
            framebuffers.push_back(vk::raii::Framebuffer(device, framebuffer_create_info));
        }
    }
} // namespace Meow
