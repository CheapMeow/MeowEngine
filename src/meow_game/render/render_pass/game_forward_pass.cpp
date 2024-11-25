#include "game_forward_pass.h"

#include "meow_runtime/pch.h"

namespace Meow
{
    GameForwardPass::GameForwardPass(const vk::raii::PhysicalDevice& physical_device,
                                     const vk::raii::Device&         logical_device,
                                     SurfaceData&                    surface_data,
                                     const vk::raii::CommandPool&    command_pool,
                                     const vk::raii::Queue&          queue,
                                     DescriptorAllocatorGrowable&    descriptor_allocator)
        : ForwardPass(logical_device)
    {
        m_pass_name = "Forward Pass";

        // Create a set to store all information of attachments

        vk::Format color_format = PickSurfaceFormat(physical_device.getSurfaceFormatsKHR(*surface_data.surface)).format;
        assert(color_format != vk::Format::eUndefined);

        std::vector<vk::AttachmentDescription> attachment_descriptions = {
            // Swap chain attachment
            vk::AttachmentDescription {
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
            // Depth attachment
            vk::AttachmentDescription {
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

        // Create reference to attachment information set

        vk::AttachmentReference swapchain_attachment_reference {
            .attachment = 0,
            .layout     = vk::ImageLayout::eColorAttachmentOptimal,
        };

        vk::AttachmentReference depth_attachment_reference {
            .attachment = 1,
            .layout     = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        };

        // Create subpass

        std::vector<vk::SubpassDescription> subpass_descriptions = {vk::SubpassDescription {
            .flags                   = vk::SubpassDescriptionFlags(),
            .pipelineBindPoint       = vk::PipelineBindPoint::eGraphics,
            .inputAttachmentCount    = 0,
            .pInputAttachments       = nullptr,
            .colorAttachmentCount    = 1,
            .pColorAttachments       = &swapchain_attachment_reference,
            .pResolveAttachments     = nullptr,
            .pDepthStencilAttachment = &depth_attachment_reference,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments    = nullptr,
        }};

        // Create subpass dependencies

        std::vector<vk::SubpassDependency> dependencies = {
            // External -> forward pass
            vk::SubpassDependency {
                .srcSubpass      = VK_SUBPASS_EXTERNAL,
                .dstSubpass      = 0,
                .srcStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe,
                .dstStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                .srcAccessMask   = vk::AccessFlagBits::eMemoryRead,
                .dstAccessMask   = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead,
                .dependencyFlags = vk::DependencyFlagBits::eByRegion,
            },
            // Forward -> external
            vk::SubpassDependency {
                .srcSubpass      = 0,
                .dstSubpass      = VK_SUBPASS_EXTERNAL,
                .srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                .dstStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe,
                .srcAccessMask   = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead,
                .dstAccessMask   = vk::AccessFlagBits::eMemoryRead,
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

        clear_values.resize(2);
        clear_values[0].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

        CreateMaterial(physical_device, logical_device, command_pool, queue, descriptor_allocator);
    }

    void GameForwardPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_forward_mat.BindPipeline(command_buffer);
        ForwardPass::DrawOnly(command_buffer);
    }
} // namespace Meow