#include "game_deferred_pass.h"

#include "meow_runtime/pch.h"

namespace Meow
{
    GameDeferredPass::GameDeferredPass(const vk::raii::PhysicalDevice& physical_device,
                                       const vk::raii::Device&         device,
                                       SurfaceData&                    surface_data,
                                       const vk::raii::CommandPool&    command_pool,
                                       const vk::raii::Queue&          queue,
                                       DescriptorAllocatorGrowable&    m_descriptor_allocator)
        : DeferredPass(device)
    {
        m_pass_name = "Deferred Pass";

        m_pass_names[0] = m_pass_name + " - Obj2Attachment Subpass";
        m_pass_names[1] = m_pass_name + " - Quad Subpass";

        // Create a set to store all information of attachments

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*surface_data.surface)).format;
        assert(color_format != vk::Format::eUndefined);

        m_color_format = color_format;

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
        // color attachment
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
                                             vk::ImageLayout::eColorAttachmentOptimal); /* finalLayout */
        // normal attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                                             /* flags */
                                             vk::Format::eR8G8B8A8Unorm,
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
                                             vk::ImageLayout::eColorAttachmentOptimal); /* finalLayout */
        // position attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),
                                             /* flags */
                                             vk::Format::eR16G16B16A16Sfloat,
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
                                             vk::ImageLayout::eColorAttachmentOptimal); /* finalLayout */
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

        std::vector<vk::AttachmentReference> color_attachment_references;
        color_attachment_references.emplace_back(1, vk::ImageLayout::eColorAttachmentOptimal);
        color_attachment_references.emplace_back(2, vk::ImageLayout::eColorAttachmentOptimal);
        color_attachment_references.emplace_back(3, vk::ImageLayout::eColorAttachmentOptimal);

        vk::AttachmentReference depth_attachment_reference(4, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        std::vector<vk::AttachmentReference> input_attachment_references;
        input_attachment_references.emplace_back(1, vk::ImageLayout::eShaderReadOnlyOptimal);
        input_attachment_references.emplace_back(2, vk::ImageLayout::eShaderReadOnlyOptimal);
        input_attachment_references.emplace_back(3, vk::ImageLayout::eShaderReadOnlyOptimal);
        input_attachment_references.emplace_back(4, vk::ImageLayout::eShaderReadOnlyOptimal);

        // Create subpass

        std::vector<vk::SubpassDescription> subpass_descriptions;
        // obj2attachment pass
        subpass_descriptions.push_back(vk::SubpassDescription(vk::SubpassDescriptionFlags(),
                                                              /* flags */
                                                              vk::PipelineBindPoint::eGraphics,
                                                              /* pipelineBindPoint */
                                                              {},
                                                              /* pInputAttachments */
                                                              color_attachment_references,
                                                              /* pColorAttachments */
                                                              {},
                                                              /* pResolveAttachments */
                                                              &depth_attachment_reference,
                                                              /* pDepthStencilAttachment */
                                                              nullptr)); /* pPreserveAttachments */
        // quad renderering pass
        subpass_descriptions.push_back(vk::SubpassDescription(vk::SubpassDescriptionFlags(),
                                                              /* flags */
                                                              vk::PipelineBindPoint::eGraphics,
                                                              /* pipelineBindPoint */
                                                              input_attachment_references,
                                                              /* pInputAttachments */
                                                              swapchain_attachment_reference,
                                                              /* pColorAttachments */
                                                              {},
                                                              /* pResolveAttachments */
                                                              {},
                                                              /* pDepthStencilAttachment */
                                                              nullptr)); /* pPreserveAttachments */

        // Create subpass dependency

        std::vector<vk::SubpassDependency> dependencies;
        // externel -> obj2attachment pass
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
                                  vk::AccessFlagBits::eColorAttachmentWrite,
                                  /* dstAccessMask */
                                  vk::DependencyFlagBits::eByRegion); /* dependencyFlags */
        // obj2attachment pass -> quad renderering pass
        dependencies.emplace_back(0,
                                  /* srcSubpass */
                                  1,
                                  /* dstSubpass */
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                  /* srcStageMask */
                                  vk::PipelineStageFlagBits::eFragmentShader,
                                  /* dstStageMask */
                                  vk::AccessFlagBits::eColorAttachmentWrite,
                                  /* srcAccessMask */
                                  vk::AccessFlagBits::eShaderRead,
                                  /* dstAccessMask */
                                  vk::DependencyFlagBits::eByRegion); /* dependencyFlags */
        // quad renderering pass -> externel
        dependencies.emplace_back(1,
                                  /* srcSubpass */
                                  VK_SUBPASS_EXTERNAL,
                                  /* dstSubpass */
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                  /* srcStageMask */
                                  vk::PipelineStageFlagBits::eFragmentShader,
                                  /* dstStageMask */
                                  vk::AccessFlagBits::eColorAttachmentWrite,
                                  /* srcAccessMask */
                                  vk::AccessFlagBits::eShaderRead,
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

        clear_values.resize(5);
        clear_values[0].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[1].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[2].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[3].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[4].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

        CreateMaterial(physical_device, device, command_pool, queue, m_descriptor_allocator);
    }

    void GameDeferredPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_obj2attachment_mat.BindPipeline(command_buffer);

        DrawObjOnly(command_buffer);

        command_buffer.nextSubpass(vk::SubpassContents::eInline);

        m_quad_mat.BindPipeline(command_buffer);

        DrawQuadOnly(command_buffer);
    }
} // namespace Meow