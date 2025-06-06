#include "forward_pass_editor.h"

#include "meow_runtime/pch.h"

#include "function/render/utils/vulkan_debug_utils.h"
#include "global/editor_context.h"
#include "meow_runtime/function/global/runtime_context.h"

namespace Meow
{
    ForwardPassEditor::ForwardPassEditor(SurfaceData& surface_data)
        : ForwardPassBase(surface_data)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        CreateRenderPass();
        CreateMaterial();
        CreateQueryPool(logical_device, 2);

        m_render_stat[0].vertex_attribute_metas = m_opaque_material->shader->vertex_attribute_metas;
        m_render_stat[0].buffer_meta_map        = m_opaque_material->shader->buffer_meta_map;
        m_render_stat[0].image_meta_map         = m_opaque_material->shader->image_meta_map;

        m_render_stat[1].vertex_attribute_metas = m_skybox_material->shader->vertex_attribute_metas;
        m_render_stat[1].buffer_meta_map        = m_skybox_material->shader->buffer_meta_map;
        m_render_stat[1].image_meta_map         = m_skybox_material->shader->image_meta_map;
    }

    void ForwardPassEditor::CreateRenderPass()
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        m_pass_names[0] = "Mesh Lighting Subpass";
        m_pass_names[1] = "Skybox Subpass";

        // Create a set to store all information of attachments

        vk::SampleCountFlagBits sample_count = g_runtime_context.render_system->GetMSAASamples();

        std::vector<vk::AttachmentDescription> attachment_descriptions;

        if (m_msaa_enabled)
        {
            attachment_descriptions = {
                // color msaa attachment
                {
                    vk::AttachmentDescriptionFlags(),         /* flags */
                    m_color_format,                           /* format */
                    sample_count,                             /* samples */
                    vk::AttachmentLoadOp::eClear,             /* loadOp */
                    vk::AttachmentStoreOp::eStore,            /* storeOp */
                    vk::AttachmentLoadOp::eDontCare,          /* stencilLoadOp */
                    vk::AttachmentStoreOp::eDontCare,         /* stencilStoreOp */
                    vk::ImageLayout::eUndefined,              /* initialLayout */
                    vk::ImageLayout::eColorAttachmentOptimal, /* finalLayout */
                },
                // depth msaa attachment
                {
                    vk::AttachmentDescriptionFlags(),                /* flags */
                    m_depth_format,                                  /* format */
                    sample_count,                                    /* samples */
                    vk::AttachmentLoadOp::eClear,                    /* loadOp */
                    vk::AttachmentStoreOp::eDontCare,                /* storeOp */
                    vk::AttachmentLoadOp::eDontCare,                 /* stencilLoadOp */
                    vk::AttachmentStoreOp::eDontCare,                /* stencilStoreOp */
                    vk::ImageLayout::eUndefined,                     /* initialLayout */
                    vk::ImageLayout::eDepthStencilAttachmentOptimal, /* finalLayout */
                },
                // offscreen attachment
                {
                    vk::AttachmentDescriptionFlags(),        /* flags */
                    m_color_format,                          /* format */
                    vk::SampleCountFlagBits::e1,             /* samples */
                    vk::AttachmentLoadOp::eClear,            /* loadOp */
                    vk::AttachmentStoreOp::eStore,           /* storeOp */
                    vk::AttachmentLoadOp::eDontCare,         /* stencilLoadOp */
                    vk::AttachmentStoreOp::eDontCare,        /* stencilStoreOp */
                    vk::ImageLayout::eShaderReadOnlyOptimal, /* initialLayout */
                    vk::ImageLayout::eShaderReadOnlyOptimal, /* finalLayout */
                },
            };
        }
        else
        {
            attachment_descriptions = {
                // offscreen attachment
                {
                    vk::AttachmentDescriptionFlags(),        /* flags */
                    m_color_format,                          /* format */
                    vk::SampleCountFlagBits::e1,             /* samples */
                    vk::AttachmentLoadOp::eClear,            /* loadOp */
                    vk::AttachmentStoreOp::eStore,           /* storeOp */
                    vk::AttachmentLoadOp::eDontCare,         /* stencilLoadOp */
                    vk::AttachmentStoreOp::eDontCare,        /* stencilStoreOp */
                    vk::ImageLayout::eShaderReadOnlyOptimal, /* initialLayout */
                    vk::ImageLayout::eShaderReadOnlyOptimal, /* finalLayout */
                },
                // depth attachment
                {
                    vk::AttachmentDescriptionFlags(),                /* flags */
                    m_depth_format,                                  /* format */
                    vk::SampleCountFlagBits::e1,                     /* samples */
                    vk::AttachmentLoadOp::eClear,                    /* loadOp */
                    vk::AttachmentStoreOp::eStore,                   /* storeOp */
                    vk::AttachmentLoadOp::eClear,                    /* stencilLoadOp */
                    vk::AttachmentStoreOp::eStore,                   /* stencilStoreOp */
                    vk::ImageLayout::eUndefined,                     /* initialLayout */
                    vk::ImageLayout::eDepthStencilAttachmentOptimal, /* finalLayout */
                },
            };
        }

        vk::AttachmentReference color_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);
        vk::AttachmentReference depth_attachment_reference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
        vk::AttachmentReference resolve_attachment_reference(2, vk::ImageLayout::eColorAttachmentOptimal);

        std::vector<vk::SubpassDescription> subpass_descriptions {
            {
                // forward pass
                vk::SubpassDescriptionFlags(),                            /* flags */
                vk::PipelineBindPoint::eGraphics,                         /* pipelineBindPoint */
                0,                                                        /* inputAttachmentCount */
                nullptr,                                                  /* pInputAttachments */
                1,                                                        /* colorAttachmentCount */
                &color_attachment_reference,                              /* pColorAttachments */
                m_msaa_enabled ? &resolve_attachment_reference : nullptr, /* pResolveAttachments */
                &depth_attachment_reference,                              /* pDepthStencilAttachment */
                0,                                                        /* preserveAttachmentCount */
                nullptr,                                                  /* pPreserveAttachments */
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
            // forward pass -> externel
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

        if (m_msaa_enabled)
        {
            clear_values.resize(3);
            clear_values[0].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
            clear_values[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
            clear_values[2].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        }
        else
        {
            clear_values.resize(2);
            clear_values[0].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
            clear_values[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
        }

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::DebugUtilsObjectNameInfoEXT name_info = {vk::ObjectType::eRenderPass,
                                                     NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkRenderPass, *render_pass),
                                                     "Forward RenderPass"};
        logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif
    }

    void
    ForwardPassEditor::Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index)
    {
        ResetQueryPool(command_buffer);
        ForwardPassBase::Start(command_buffer, extent, image_index);
    }

    void ForwardPassEditor::RecordGraphicsCommand(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index)
    {
        FUNCTION_TIMER();

        m_opaque_material->BindPipeline(command_buffer);

        BeginQuery(command_buffer);
        RenderOpaqueMeshes(command_buffer);
        EndQuery(command_buffer);

        m_skybox_material->BindPipeline(command_buffer);

        BeginQuery(command_buffer);
        RenderSkybox(command_buffer);
        EndQuery(command_buffer);

        m_translucent_material->BindPipeline(command_buffer);
        RenderTranslucentMeshes(command_buffer);
    }

    void ForwardPassEditor::AfterPresent()
    {
        FUNCTION_TIMER();

        if (m_query_enabled)
        {
            for (int i = 1; i >= 0; i--)
            {
                std::pair<vk::Result, std::vector<uint32_t>> query_results = GetQueryResults(i);
                g_editor_context.profile_system->UploadPipelineStat(m_pass_names[i], query_results.second);
            }
        }

        for (int i = 1; i >= 0; i--)
        {
            m_render_stat[i].draw_call = draw_call[i];
            g_editor_context.profile_system->UploadBuiltinRenderStat(m_pass_names[i], m_render_stat[i]);
        }
    }

    void swap(ForwardPassEditor& lhs, ForwardPassEditor& rhs)
    {
        using std::swap;

        swap(static_cast<ForwardPassBase&>(lhs), static_cast<ForwardPassBase&>(rhs));
        swap(static_cast<PipelineQueryable&>(lhs), static_cast<PipelineQueryable&>(rhs));

        swap(lhs.m_render_stat, rhs.m_render_stat);
    }
} // namespace Meow
