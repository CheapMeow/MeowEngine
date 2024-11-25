#include "game_deferred_pass.h"

#include "meow_runtime/pch.h"

namespace Meow
{
    GameDeferredPass::GameDeferredPass(const vk::raii::PhysicalDevice& physical_device,
                                       const vk::raii::Device&         logical_device,
                                       SurfaceData&                    surface_data,
                                       const vk::raii::CommandPool&    command_pool,
                                       const vk::raii::Queue&          queue,
                                       DescriptorAllocatorGrowable&    descriptor_allocator)
        : DeferredPass(logical_device)
    {
        m_pass_name = "Deferred Pass";

        m_pass_names[0] = m_pass_name + " - Obj2Attachment Subpass";
        m_pass_names[1] = m_pass_name + " - Quad Subpass";

        // Create a set to store all information of attachments

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*surface_data.surface)).format;
        assert(color_format != vk::Format::eUndefined);

        m_color_format = color_format;

        std::vector<vk::AttachmentDescription> attachment_descriptions = {
            // swap chain attachment
            {
                .flags          = vk::AttachmentDescriptionFlags(),
                .format         = color_format,
                .samples        = m_sample_count,
                .loadOp         = vk::AttachmentLoadOp::eClear,
                .storeOp        = vk::AttachmentStoreOp::eStore,
                .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
                .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                .initialLayout  = vk::ImageLayout::eUndefined,
                .finalLayout    = vk::ImageLayout::ePresentSrcKHR,
            },
            // color attachment
            {
                .flags          = vk::AttachmentDescriptionFlags(),
                .format         = color_format,
                .samples        = m_sample_count,
                .loadOp         = vk::AttachmentLoadOp::eClear,
                .storeOp        = vk::AttachmentStoreOp::eStore,
                .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
                .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                .initialLayout  = vk::ImageLayout::eUndefined,
                .finalLayout    = vk::ImageLayout::eColorAttachmentOptimal,
            },
            // normal attachment
            {
                .flags          = vk::AttachmentDescriptionFlags(),
                .format         = vk::Format::eR8G8B8A8Unorm,
                .samples        = m_sample_count,
                .loadOp         = vk::AttachmentLoadOp::eClear,
                .storeOp        = vk::AttachmentStoreOp::eStore,
                .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
                .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                .initialLayout  = vk::ImageLayout::eUndefined,
                .finalLayout    = vk::ImageLayout::eColorAttachmentOptimal,
            },
            // position attachment
            {
                .flags          = vk::AttachmentDescriptionFlags(),
                .format         = vk::Format::eR16G16B16A16Sfloat,
                .samples        = m_sample_count,
                .loadOp         = vk::AttachmentLoadOp::eClear,
                .storeOp        = vk::AttachmentStoreOp::eStore,
                .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
                .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                .initialLayout  = vk::ImageLayout::eUndefined,
                .finalLayout    = vk::ImageLayout::eColorAttachmentOptimal,
            },
            // depth attachment
            {
                .flags          = vk::AttachmentDescriptionFlags(),
                .format         = m_depth_format,
                .samples        = m_sample_count,
                .loadOp         = vk::AttachmentLoadOp::eClear,
                .storeOp        = vk::AttachmentStoreOp::eStore,
                .stencilLoadOp  = vk::AttachmentLoadOp::eClear,
                .stencilStoreOp = vk::AttachmentStoreOp::eStore,
                .initialLayout  = vk::ImageLayout::eUndefined,
                .finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            },
        };

        vk::AttachmentReference swapchain_attachment_reference = {
            .attachment = 0,
            .layout     = vk::ImageLayout::eColorAttachmentOptimal,
        };

        std::vector<vk::AttachmentReference> color_attachment_references = {
            {1, vk::ImageLayout::eColorAttachmentOptimal},
            {2, vk::ImageLayout::eColorAttachmentOptimal},
            {3, vk::ImageLayout::eColorAttachmentOptimal},
        };

        vk::AttachmentReference depth_attachment_reference = {
            .attachment = 4,
            .layout     = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        };

        std::vector<vk::AttachmentReference> input_attachment_references = {
            {1, vk::ImageLayout::eShaderReadOnlyOptimal},
            {2, vk::ImageLayout::eShaderReadOnlyOptimal},
            {3, vk::ImageLayout::eShaderReadOnlyOptimal},
            {4, vk::ImageLayout::eShaderReadOnlyOptimal},
        };

        std::vector<vk::SubpassDescription> subpass_descriptions = {
            // obj2attachment pass
            {
                .pipelineBindPoint       = vk::PipelineBindPoint::eGraphics,
                .colorAttachmentCount    = static_cast<uint32_t>(color_attachment_references.size()),
                .pColorAttachments       = color_attachment_references.data(),
                .pDepthStencilAttachment = &depth_attachment_reference,
            },
            // quad rendering pass
            {
                .pipelineBindPoint    = vk::PipelineBindPoint::eGraphics,
                .inputAttachmentCount = static_cast<uint32_t>(input_attachment_references.size()),
                .pInputAttachments    = input_attachment_references.data(),
                .colorAttachmentCount = 1,
                .pColorAttachments    = &swapchain_attachment_reference,
            },
        };

        std::vector<vk::SubpassDependency> dependencies = {
            // external -> obj2attachment pass
            {
                .srcSubpass      = VK_SUBPASS_EXTERNAL,
                .dstSubpass      = 0,
                .srcStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe,
                .dstStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                .srcAccessMask   = vk::AccessFlagBits::eMemoryRead,
                .dstAccessMask   = vk::AccessFlagBits::eColorAttachmentWrite,
                .dependencyFlags = vk::DependencyFlagBits::eByRegion,
            },
            // obj2attachment pass -> quad rendering pass
            {
                .srcSubpass      = 0,
                .dstSubpass      = 1,
                .srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                .dstStageMask    = vk::PipelineStageFlagBits::eFragmentShader,
                .srcAccessMask   = vk::AccessFlagBits::eColorAttachmentWrite,
                .dstAccessMask   = vk::AccessFlagBits::eShaderRead,
                .dependencyFlags = vk::DependencyFlagBits::eByRegion,
            },
            // quad rendering pass -> external
            {
                .srcSubpass      = 1,
                .dstSubpass      = VK_SUBPASS_EXTERNAL,
                .srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                .dstStageMask    = vk::PipelineStageFlagBits::eFragmentShader,
                .srcAccessMask   = vk::AccessFlagBits::eColorAttachmentWrite,
                .dstAccessMask   = vk::AccessFlagBits::eShaderRead,
                .dependencyFlags = vk::DependencyFlagBits::eByRegion,
            },
        };

        // Create render pass
        vk::RenderPassCreateInfo render_pass_create_info = {
            .attachmentCount = static_cast<uint32_t>(attachment_descriptions.size()),
            .pAttachments    = attachment_descriptions.data(),
            .subpassCount    = static_cast<uint32_t>(subpass_descriptions.size()),
            .pSubpasses      = subpass_descriptions.data(),
            .dependencyCount = static_cast<uint32_t>(dependencies.size()),
            .pDependencies   = dependencies.data(),
        };

        render_pass = vk::raii::RenderPass(logical_device, render_pass_create_info);

        clear_values.resize(5);
        clear_values[0].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[1].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[2].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[3].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[4].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

        CreateMaterial(physical_device, logical_device, command_pool, queue, descriptor_allocator);
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