#include "editor_forward_pass.h"

#include "meow_runtime/pch.h"

#include "global/editor_context.h"
#include "meow_runtime/function/global/runtime_context.h"

namespace Meow
{
    EditorForwardPass::EditorForwardPass(SurfaceData& surface_data)
        : ForwardPass()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

#ifdef MEOW_EDITOR
        m_pass_names[0] = "Mesh Lighting Subpass";
        m_pass_names[1] = "Skybox Subpass";
#endif

        // Create a set to store all information of attachments

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*surface_data.surface)).format;
        assert(color_format != vk::Format::eUndefined);

        std::vector<vk::AttachmentDescription> attachment_descriptions {
#ifdef MEOW_EDITOR
            // offscreen attachment
            {
                vk::AttachmentDescriptionFlags(),        /* flags */
                color_format,                            /* format */
                m_sample_count,                          /* samples */
                vk::AttachmentLoadOp::eClear,            /* loadOp */
                vk::AttachmentStoreOp::eStore,           /* storeOp */
                vk::AttachmentLoadOp::eDontCare,         /* stencilLoadOp */
                vk::AttachmentStoreOp::eDontCare,        /* stencilStoreOp */
                vk::ImageLayout::eShaderReadOnlyOptimal, /* initialLayout */
                vk::ImageLayout::eShaderReadOnlyOptimal, /* finalLayout */
            },
#else
            {
                vk::AttachmentDescriptionFlags(), /* flags */
                color_format,                     /* format */
                m_sample_count,                   /* samples */
                vk::AttachmentLoadOp::eClear,     /* loadOp */
                vk::AttachmentStoreOp::eStore,    /* storeOp */
                vk::AttachmentLoadOp::eDontCare,  /* stencilLoadOp */
                vk::AttachmentStoreOp::eDontCare, /* stencilStoreOp */
                vk::ImageLayout::eUndefined,      /* initialLayout */
                vk::ImageLayout::ePresentSrcKHR,  /* finalLayout */
            },
#endif
            // depth attachment
            {
                vk::AttachmentDescriptionFlags(),                /* flags */
                m_depth_format,                                  /* format */
                m_sample_count,                                  /* samples */
                vk::AttachmentLoadOp::eClear,                    /* loadOp */
                vk::AttachmentStoreOp::eStore,                   /* storeOp */
                vk::AttachmentLoadOp::eClear,                    /* stencilLoadOp */
                vk::AttachmentStoreOp::eStore,                   /* stencilStoreOp */
                vk::ImageLayout::eUndefined,                     /* initialLayout */
                vk::ImageLayout::eDepthStencilAttachmentOptimal, /* finalLayout */
            },
        };

        vk::AttachmentReference swapchain_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);
        vk::AttachmentReference depth_attachment_reference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        std::vector<vk::SubpassDescription> subpass_descriptions {
            // forward pass
            {
                vk::SubpassDescriptionFlags(),    /* flags */
                vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                {},                               /* pInputAttachments */
                swapchain_attachment_reference,   /* pColorAttachments */
                {},                               /* pResolveAttachments */
                &depth_attachment_reference,      /* pDepthStencilAttachment */
                nullptr,                          /* pPreserveAttachments */
            },
        };

        std::vector<vk::SubpassDependency> dependencies {
            // externel -> forward pass
            {
                VK_SUBPASS_EXTERNAL,                               /* srcSubpass */
                0,                                                 /* dstSubpass */
                vk::PipelineStageFlagBits::eBottomOfPipe,          /* srcStageMask */
                vk::PipelineStageFlagBits::eColorAttachmentOutput, /* dstStageMask */
                vk::AccessFlagBits::eMemoryRead,                   /* srcAccessMask */
                vk::AccessFlagBits::eColorAttachmentWrite |
                    vk::AccessFlagBits::eColorAttachmentRead, /* dstAccessMask */
                vk::DependencyFlagBits::eByRegion,            /* dependencyFlags */
            },
            // forward -> externel
            {
                0,                                                 /* srcSubpass */
                VK_SUBPASS_EXTERNAL,                               /* dstSubpass */
                vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                vk::PipelineStageFlagBits::eBottomOfPipe,          /* dstStageMask */
                vk::AccessFlagBits::eColorAttachmentWrite |
                    vk::AccessFlagBits::eColorAttachmentRead, /* srcAccessMask */
                vk::AccessFlagBits::eMemoryRead,              /* dstAccessMask */
                vk::DependencyFlagBits::eByRegion,            /* dependencyFlags */
            },
        };

        // Create render pass
        vk::RenderPassCreateInfo render_pass_create_info(vk::RenderPassCreateFlags(), /* flags */
                                                         attachment_descriptions,     /* pAttachments */
                                                         subpass_descriptions,        /* pSubpasses */
                                                         dependencies);               /* pDependencies */

        render_pass = vk::raii::RenderPass(logical_device, render_pass_create_info);

        clear_values.resize(2);
        clear_values[0].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

        CreateMaterial();

#ifdef MEOW_EDITOR
        VkQueryPoolCreateInfo query_pool_create_info = {.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                                        .queryType          = VK_QUERY_TYPE_PIPELINE_STATISTICS,
                                                        .queryCount         = 2,
                                                        .pipelineStatistics = (1 << 11) - 1};

        query_pool = logical_device.createQueryPool(query_pool_create_info, nullptr);

        m_render_stat[0].vertex_attribute_metas = m_forward_mat.shader_ptr->vertex_attribute_metas;
        m_render_stat[0].buffer_meta_map        = m_forward_mat.shader_ptr->buffer_meta_map;
        m_render_stat[0].image_meta_map         = m_forward_mat.shader_ptr->image_meta_map;

        m_render_stat[1].vertex_attribute_metas = m_skybox_mat.shader_ptr->vertex_attribute_metas;
        m_render_stat[1].buffer_meta_map        = m_skybox_mat.shader_ptr->buffer_meta_map;
        m_render_stat[1].image_meta_map         = m_skybox_mat.shader_ptr->image_meta_map;
#endif
    }

    void EditorForwardPass::Start(const vk::raii::CommandBuffer& command_buffer,
                                  vk::Extent2D                   extent,
                                  uint32_t                       current_image_index)
    {
#ifdef MEOW_EDITOR
        if (m_query_enabled)
            command_buffer.resetQueryPool(*query_pool, 0, 2);
#endif

        ForwardPass::Start(command_buffer, extent, current_image_index);
    }

    void EditorForwardPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_forward_mat.BindPipeline(command_buffer);

#ifdef MEOW_EDITOR
        if (m_query_enabled)
            command_buffer.beginQuery(*query_pool, 0, {});
#endif

        MeshLighting(command_buffer);

#ifdef MEOW_EDITOR
        if (m_query_enabled)
            command_buffer.endQuery(*query_pool, 0);
#endif

        m_skybox_mat.BindPipeline(command_buffer);

#ifdef MEOW_EDITOR
        if (m_query_enabled)
            command_buffer.beginQuery(*query_pool, 1, {});
#endif

        RenderSkybox(command_buffer);

#ifdef MEOW_EDITOR
        if (m_query_enabled)
            command_buffer.endQuery(*query_pool, 1);
#endif
    }

    void EditorForwardPass::AfterPresent()
    {
        FUNCTION_TIMER();

#ifdef MEOW_EDITOR
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
#endif
    }

    void swap(EditorForwardPass& lhs, EditorForwardPass& rhs)
    {
        using std::swap;

#ifdef MEOW_EDITOR
        swap(lhs.m_query_enabled, rhs.m_query_enabled);
        swap(lhs.query_pool, rhs.query_pool);
        swap(lhs.m_render_stat, rhs.m_render_stat);
#endif
    }
} // namespace Meow
