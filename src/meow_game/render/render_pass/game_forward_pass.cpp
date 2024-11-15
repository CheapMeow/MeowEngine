#include "game_forward_pass.h"

#include "meow_runtime/pch.h"

namespace Meow
{
    GameForwardPass::GameForwardPass(const vk::raii::PhysicalDevice& physical_device,
                                     const vk::raii::Device&         device,
                                     SurfaceData&                    surface_data,
                                     const vk::raii::CommandPool&    command_pool,
                                     const vk::raii::Queue&          queue,
                                     DescriptorAllocatorGrowable&    m_descriptor_allocator)
        : ForwardPass(device)
    {
        m_pass_name = "Forward Pass";

        // Create a set to store all information of attachments

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*surface_data.surface)).format;
        assert(color_format != vk::Format::eUndefined);

        std::vector<vk::AttachmentDescription> attachment_descriptions;
        // swap chain attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                                             /* flags */
                                             color_format,
                                             /* format */
                                             m_sample_count,
                                             /* samples */
                                             vk::AttachmentLoadOp::eClear,
                                             /* loadOp */
                                             vk::AttachmentStoreOp::eStore,
                                             /* storeOp */
                                             vk::AttachmentLoadOp::eDontCare,
                                             /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eDontCare,
                                             /* stencilStoreOp */
                                             vk::ImageLayout::eUndefined,
                                             /* initialLayout */
                                             vk::ImageLayout::ePresentSrcKHR); /* finalLayout */
        // depth attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                                             /* flags */
                                             m_depth_format,
                                             /* format */
                                             m_sample_count,
                                             /* samples */
                                             vk::AttachmentLoadOp::eClear,
                                             /* loadOp */
                                             vk::AttachmentStoreOp::eStore,
                                             /* storeOp */
                                             vk::AttachmentLoadOp::eClear,
                                             /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eStore,
                                             /* stencilStoreOp */
                                             vk::ImageLayout::eUndefined,
                                             /* initialLayout */
                                             vk::ImageLayout::eDepthStencilAttachmentOptimal); /* finalLayout */

        // Create reference to attachment information set

        vk::AttachmentReference swapchain_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);
        vk::AttachmentReference depth_attachment_reference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        // Create subpass

        std::vector<vk::SubpassDescription> subpass_descriptions;
        // obj2attachment pass
        subpass_descriptions.push_back(vk::SubpassDescription(vk::SubpassDescriptionFlags(),
                                                              /* flags */
                                                              vk::PipelineBindPoint::eGraphics,
                                                              /* pipelineBindPoint */
                                                              {},
                                                              /* pInputAttachments */
                                                              swapchain_attachment_reference,
                                                              /* pColorAttachments */
                                                              {},
                                                              /* pResolveAttachments */
                                                              &depth_attachment_reference,
                                                              /* pDepthStencilAttachment */
                                                              nullptr)); /* pPreserveAttachments */

        // Create subpass dependency

        std::vector<vk::SubpassDependency> dependencies;
        // externel -> forward pass
        dependencies.emplace_back(VK_SUBPASS_EXTERNAL,
                                  /* srcSubpass */
                                  0,
                                  /* dstSubpass */
                                  vk::PipelineStageFlagBits::eBottomOfPipe,
                                  /* srcStageMask */
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                  /* dstStageMask */
                                  vk::AccessFlagBits::eMemoryRead,
                                  /* srcAccessMask */
                                  vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead,
                                  /* dstAccessMask */
                                  vk::DependencyFlagBits::eByRegion); /* dependencyFlags */
        // forward -> externel
        dependencies.emplace_back(0,
                                  /* srcSubpass */
                                  VK_SUBPASS_EXTERNAL,
                                  /* dstSubpass */
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                  /* srcStageMask */
                                  vk::PipelineStageFlagBits::eBottomOfPipe,
                                  /* dstStageMask */
                                  vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead,
                                  /* srcAccessMask */
                                  vk::AccessFlagBits::eMemoryRead,
                                  /* dstAccessMask */
                                  vk::DependencyFlagBits::eByRegion); /* dependencyFlags */

        // Create render pass
        vk::RenderPassCreateInfo render_pass_create_info(vk::RenderPassCreateFlags(),
                                                         /* flags */
                                                         attachment_descriptions,
                                                         /* pAttachments */
                                                         subpass_descriptions,
                                                         /* pSubpasses */
                                                         dependencies); /* pDependencies */

        render_pass = vk::raii::RenderPass(device, render_pass_create_info);

        clear_values.resize(2);
        clear_values[0].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

        CreateMaterial(physical_device, device, m_descriptor_allocator);
    }

    void GameForwardPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_forward_mat.BindPipeline(command_buffer);
        ForwardPass::DrawOnly(command_buffer);
    }
} // namespace Meow