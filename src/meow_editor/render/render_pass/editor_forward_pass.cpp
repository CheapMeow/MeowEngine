#include "editor_forward_pass.h"

#include "meow_runtime/pch.h"

#include "global/editor_context.h"

namespace Meow
{
    EditorForwardPass::EditorForwardPass(const vk::raii::PhysicalDevice& physical_device,
                                         const vk::raii::Device&         logical_device,
                                         SurfaceData&                    surface_data,
                                         const vk::raii::CommandPool&    command_pool,
                                         const vk::raii::Queue&          queue,
                                         DescriptorAllocatorGrowable&    descriptor_allocator)
        : ForwardPass(logical_device)
    {
        m_pass_name = "Forward Pass";

        // Create a set to store all information of attachments

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*surface_data.surface)).format;
        assert(color_format != vk::Format::eUndefined);

        std::vector<vk::AttachmentDescription> attachment_descriptions;
        // swap chain attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),         /* flags */
                                             color_format,                             /* format */
                                             m_sample_count,                           /* samples */
                                             vk::AttachmentLoadOp::eClear,             /* loadOp */
                                             vk::AttachmentStoreOp::eStore,            /* storeOp */
                                             vk::AttachmentLoadOp::eDontCare,          /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eDontCare,         /* stencilStoreOp */
                                             vk::ImageLayout::eShaderReadOnlyOptimal,  /* initialLayout */
                                             vk::ImageLayout::eShaderReadOnlyOptimal); /* finalLayout */
        // depth attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),                 /* flags */
                                             m_depth_format,                                   /* format */
                                             m_sample_count,                                   /* samples */
                                             vk::AttachmentLoadOp::eClear,                     /* loadOp */
                                             vk::AttachmentStoreOp::eStore,                    /* storeOp */
                                             vk::AttachmentLoadOp::eClear,                     /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eStore,                    /* stencilStoreOp */
                                             vk::ImageLayout::eUndefined,                      /* initialLayout */
                                             vk::ImageLayout::eDepthStencilAttachmentOptimal); /* finalLayout */

        // Create reference to attachment information set

        vk::AttachmentReference swapchain_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);
        vk::AttachmentReference depth_attachment_reference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        // Create subpass

        std::vector<vk::SubpassDescription> subpass_descriptions;
        // obj2attachment pass
        subpass_descriptions.push_back(vk::SubpassDescription(vk::SubpassDescriptionFlags(),    /* flags */
                                                              vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                                                              {},                               /* pInputAttachments */
                                                              swapchain_attachment_reference,   /* pColorAttachments */
                                                              {},                          /* pResolveAttachments */
                                                              &depth_attachment_reference, /* pDepthStencilAttachment */
                                                              nullptr));                   /* pPreserveAttachments */

        // Create subpass dependency

        std::vector<vk::SubpassDependency> dependencies;
        // externel -> forward pass
        dependencies.emplace_back(VK_SUBPASS_EXTERNAL,                               /* srcSubpass */
                                  0,                                                 /* dstSubpass */
                                  vk::PipelineStageFlagBits::eBottomOfPipe,          /* srcStageMask */
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput, /* dstStageMask */
                                  vk::AccessFlagBits::eMemoryRead,                   /* srcAccessMask */
                                  vk::AccessFlagBits::eColorAttachmentWrite |
                                      vk::AccessFlagBits::eColorAttachmentRead, /* dstAccessMask */
                                  vk::DependencyFlagBits::eByRegion);           /* dependencyFlags */
        // forward -> externel
        dependencies.emplace_back(0,                                                 /* srcSubpass */
                                  VK_SUBPASS_EXTERNAL,                               /* dstSubpass */
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                                  vk::PipelineStageFlagBits::eBottomOfPipe,          /* dstStageMask */
                                  vk::AccessFlagBits::eColorAttachmentWrite |
                                      vk::AccessFlagBits::eColorAttachmentRead, /* srcAccessMask */
                                  vk::AccessFlagBits::eMemoryRead,              /* dstAccessMask */
                                  vk::DependencyFlagBits::eByRegion);           /* dependencyFlags */

        // Create render pass
        vk::RenderPassCreateInfo render_pass_create_info(vk::RenderPassCreateFlags(), /* flags */
                                                         attachment_descriptions,     /* pAttachments */
                                                         subpass_descriptions,        /* pSubpasses */
                                                         dependencies);               /* pDependencies */

        render_pass = vk::raii::RenderPass(logical_device, render_pass_create_info);

        clear_values.resize(2);
        clear_values[0].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

        CreateMaterial(physical_device, logical_device, command_pool, queue, descriptor_allocator);

        // Debug

        VkQueryPoolCreateInfo query_pool_create_info = {.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                                        .queryType          = VK_QUERY_TYPE_PIPELINE_STATISTICS,
                                                        .queryCount         = 1,
                                                        .pipelineStatistics = (1 << 11) - 1};

        query_pool = logical_device.createQueryPool(query_pool_create_info, nullptr);

        m_render_stat.vertex_attribute_metas = m_forward_mat.shader_ptr->vertex_attribute_metas;
        m_render_stat.buffer_meta_map        = m_forward_mat.shader_ptr->buffer_meta_map;
        m_render_stat.image_meta_map         = m_forward_mat.shader_ptr->image_meta_map;
    }

    void EditorForwardPass::Start(const vk::raii::CommandBuffer& command_buffer,
                                  vk::Extent2D                   extent,
                                  uint32_t                       current_image_index)
    {
        if (m_query_enabled)
            command_buffer.resetQueryPool(*query_pool, 0, 1);

        ForwardPass::Start(command_buffer, extent, current_image_index);
    }

    void EditorForwardPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_forward_mat.BindPipeline(command_buffer);

        if (m_query_enabled)
            command_buffer.beginQuery(*query_pool, 0, {});

        ForwardPass::DrawOnly(command_buffer);

        if (m_query_enabled)
            command_buffer.endQuery(*query_pool, 0);
    }

    void EditorForwardPass::AfterPresent()
    {
        FUNCTION_TIMER();

        if (m_query_enabled)
        {
            std::pair<vk::Result, std::vector<uint32_t>> query_results =
                query_pool.getResults<uint32_t>(0, 1, sizeof(uint32_t) * 11, sizeof(uint32_t) * 11, {});

            g_editor_context.profile_system->UploadPipelineStat(m_pass_name, query_results.second);
        }

        m_render_stat.draw_call = draw_call;
        g_editor_context.profile_system->UploadBuiltinRenderStat(m_pass_name, m_render_stat);
    }

    void swap(EditorForwardPass& lhs, EditorForwardPass& rhs)
    {
        using std::swap;

        swap(lhs.m_query_enabled, rhs.m_query_enabled);
        swap(lhs.query_pool, rhs.query_pool);
        swap(lhs.m_render_stat, rhs.m_render_stat);
    }
} // namespace Meow
