#include "editor_deferred_pass.h"

#include "meow_runtime/pch.h"

#include "global/editor_context.h"

namespace Meow
{
    EditorDeferredPass::EditorDeferredPass(const vk::raii::PhysicalDevice& physical_device,
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
            vk::AttachmentDescription {
                .format         = color_format,
                .samples        = m_sample_count,
                .loadOp         = vk::AttachmentLoadOp::eClear,
                .storeOp        = vk::AttachmentStoreOp::eStore,
                .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
                .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                .initialLayout  = vk::ImageLayout::eShaderReadOnlyOptimal,
                .finalLayout    = vk::ImageLayout::eShaderReadOnlyOptimal,
            },
            // color attachment
            vk::AttachmentDescription {
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
            vk::AttachmentDescription {
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
            vk::AttachmentDescription {
                .format         = vk::Format::eR16G16B16A16Sfloat,
                .samples        = m_sample_count,
                .loadOp         = vk::AttachmentLoadOp::eClear,
                .storeOp        = vk::AttachmentStoreOp::eStore,
                .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
                .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                .initialLayout  = vk::ImageLayout::eUndefined,
                .finalLayout    = vk::ImageLayout::eColorAttachmentOptimal,
            },
        };

        // depth attachment
        attachment_descriptions.push_back(vk::AttachmentDescription {
            .format         = m_depth_format,
            .samples        = m_sample_count,
            .loadOp         = vk::AttachmentLoadOp::eClear,
            .storeOp        = vk::AttachmentStoreOp::eStore,
            .stencilLoadOp  = vk::AttachmentLoadOp::eClear,
            .stencilStoreOp = vk::AttachmentStoreOp::eStore,
            .initialLayout  = vk::ImageLayout::eUndefined,
            .finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        });

        // Create reference to attachment information set

        vk::AttachmentReference swapchain_attachment_reference = {
            .attachment = 0,
            .layout     = vk::ImageLayout::eColorAttachmentOptimal,
        };

        std::vector<vk::AttachmentReference> color_attachment_references = {
            {.attachment = 1, .layout = vk::ImageLayout::eColorAttachmentOptimal},
            {.attachment = 2, .layout = vk::ImageLayout::eColorAttachmentOptimal},
            {.attachment = 3, .layout = vk::ImageLayout::eColorAttachmentOptimal},
        };

        vk::AttachmentReference depth_attachment_reference = {
            .attachment = 4,
            .layout     = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        };

        std::vector<vk::AttachmentReference> input_attachment_references = {
            {.attachment = 1, .layout = vk::ImageLayout::eShaderReadOnlyOptimal},
            {.attachment = 2, .layout = vk::ImageLayout::eShaderReadOnlyOptimal},
            {.attachment = 3, .layout = vk::ImageLayout::eShaderReadOnlyOptimal},
            {.attachment = 4, .layout = vk::ImageLayout::eShaderReadOnlyOptimal},
        };

        // Create subpass

        std::vector<vk::SubpassDescription> subpass_descriptions = {
            vk::SubpassDescription {
                .pipelineBindPoint       = vk::PipelineBindPoint::eGraphics,
                .pColorAttachments       = color_attachment_references.data(),
                .pDepthStencilAttachment = &depth_attachment_reference,
            },
            vk::SubpassDescription {
                .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
                .pInputAttachments = input_attachment_references.data(),
                .pColorAttachments = &swapchain_attachment_reference,
            },
        };

        // Create subpass dependency

        std::vector<vk::SubpassDependency> dependencies = {
            vk::SubpassDependency {
                .srcSubpass      = VK_SUBPASS_EXTERNAL,
                .dstSubpass      = 0,
                .srcStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe,
                .dstStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                .srcAccessMask   = vk::AccessFlagBits::eMemoryRead,
                .dstAccessMask   = vk::AccessFlagBits::eColorAttachmentWrite,
                .dependencyFlags = vk::DependencyFlagBits::eByRegion,
            },
            vk::SubpassDependency {
                .srcSubpass      = 0,
                .dstSubpass      = 1,
                .srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                .dstStageMask    = vk::PipelineStageFlagBits::eFragmentShader,
                .srcAccessMask   = vk::AccessFlagBits::eColorAttachmentWrite,
                .dstAccessMask   = vk::AccessFlagBits::eShaderRead,
                .dependencyFlags = vk::DependencyFlagBits::eByRegion,
            },
            vk::SubpassDependency {
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

        // Debug

        vk::QueryPoolCreateInfo query_pool_create_info = {
            .queryType          = vk::QueryType::ePipelineStatistics,
            .queryCount         = 1,
            .pipelineStatistics = vk::QueryPipelineStatisticFlags {(1 << 11) - 1},
        };

        query_pool = logical_device.createQueryPool(query_pool_create_info, nullptr);

        m_render_stat[0].vertex_attribute_metas = m_obj2attachment_mat.shader_ptr->vertex_attribute_metas;
        m_render_stat[0].buffer_meta_map        = m_obj2attachment_mat.shader_ptr->buffer_meta_map;
        m_render_stat[0].image_meta_map         = m_obj2attachment_mat.shader_ptr->image_meta_map;

        m_render_stat[1].vertex_attribute_metas = m_quad_mat.shader_ptr->vertex_attribute_metas;
        m_render_stat[1].buffer_meta_map        = m_quad_mat.shader_ptr->buffer_meta_map;
        m_render_stat[1].image_meta_map         = m_quad_mat.shader_ptr->image_meta_map;
    }

    void EditorDeferredPass::Start(const vk::raii::CommandBuffer& command_buffer,
                                   vk::Extent2D                   extent,
                                   uint32_t                       current_image_index)
    {
        // Debug
        if (m_query_enabled)
        {
            for (int i = 0; i < 2; i++)
            {
                command_buffer.resetQueryPool(*query_pool, 0, 2);
            }
        }

        DeferredPass::Start(command_buffer, extent, current_image_index);
    }

    void EditorDeferredPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_obj2attachment_mat.BindPipeline(command_buffer);

        if (m_query_enabled)
            command_buffer.beginQuery(*query_pool, 0, {});

        DrawObjOnly(command_buffer);

        if (m_query_enabled)
            command_buffer.endQuery(*query_pool, 0);

        command_buffer.nextSubpass(vk::SubpassContents::eInline);

        m_quad_mat.BindPipeline(command_buffer);

        if (m_query_enabled)
            command_buffer.beginQuery(*query_pool, 1, {});

        DrawQuadOnly(command_buffer);

        if (m_query_enabled)
            command_buffer.endQuery(*query_pool, 1);
    }

    void EditorDeferredPass::AfterPresent()
    {
        FUNCTION_TIMER();

        if (m_query_enabled)
        {
            for (int i = 1; i >= 0; i--)
            {
                std::pair<vk::Result, std::vector<uint32_t>> query_results =
                    query_pool.getResults<uint32_t>(i, 1, sizeof(uint32_t) * 11, sizeof(uint32_t) * 11, {});

                g_editor_context.profile_system->UploadPipelineStat(m_pass_names[i], query_results.second);
            }
        }

        for (int i = 1; i >= 0; i--)
        {
            m_render_stat[i].draw_call = draw_call[i];
            g_editor_context.profile_system->UploadBuiltinRenderStat(m_pass_names[i], m_render_stat[i]);
        }
    }

    void swap(EditorDeferredPass& lhs, EditorDeferredPass& rhs)
    {
        using std::swap;

        swap(lhs.m_query_enabled, rhs.m_query_enabled);
        swap(lhs.query_pool, rhs.query_pool);
        swap(lhs.m_render_stat, rhs.m_render_stat);
    }
} // namespace Meow
