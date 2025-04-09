#include "deferred_pass_editor.h"

#include "meow_runtime/pch.h"

#include "global/editor_context.h"
#include "meow_runtime/function/global/runtime_context.h"

namespace Meow
{
    DeferredPassEditor::DeferredPassEditor(SurfaceData& surface_data)
        : DeferredPassBase(surface_data)
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        m_pass_names[0] = "GBuffer Subpass";
        m_pass_names[1] = "Mesh Lighting Subpass";

        // Create a set to store all information of attachments

        // TODO: multiple window
        // RenderPass should be independent from window surface data
        // But attachment should be created with certain color format
        // So when then scene is crossing two windows,
        // we should spilt rendering into two parts
        // and we should have two different set of attachments?

        vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1;

        std::vector<vk::AttachmentDescription> attachment_descriptions {
            // swap chain attachment
            {
                vk::AttachmentDescriptionFlags(),        /* flags */
                m_color_format,                          /* format */
                sample_count,                            /* samples */
                vk::AttachmentLoadOp::eClear,            /* loadOp */
                vk::AttachmentStoreOp::eStore,           /* storeOp */
                vk::AttachmentLoadOp::eDontCare,         /* stencilLoadOp */
                vk::AttachmentStoreOp::eDontCare,        /* stencilStoreOp */
                vk::ImageLayout::eShaderReadOnlyOptimal, /* initialLayout */
                vk::ImageLayout::eShaderReadOnlyOptimal, /* finalLayout */
            },
            // color attachment
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
            // normal attachment
            {
                vk::AttachmentDescriptionFlags(),         /* flags */
                vk::Format::eR8G8B8A8Unorm,               /* format */
                sample_count,                             /* samples */
                vk::AttachmentLoadOp::eClear,             /* loadOp */
                vk::AttachmentStoreOp::eStore,            /* storeOp */
                vk::AttachmentLoadOp::eDontCare,          /* stencilLoadOp */
                vk::AttachmentStoreOp::eDontCare,         /* stencilStoreOp */
                vk::ImageLayout::eUndefined,              /* initialLayout */
                vk::ImageLayout::eColorAttachmentOptimal, /* finalLayout */
            },
            // position attachment
            {
                vk::AttachmentDescriptionFlags(),         /* flags */
                vk::Format::eR16G16B16A16Sfloat,          /* format */
                sample_count,                             /* samples */
                vk::AttachmentLoadOp::eClear,             /* loadOp */
                vk::AttachmentStoreOp::eStore,            /* storeOp */
                vk::AttachmentLoadOp::eDontCare,          /* stencilLoadOp */
                vk::AttachmentStoreOp::eDontCare,         /* stencilStoreOp */
                vk::ImageLayout::eUndefined,              /* initialLayout */
                vk::ImageLayout::eColorAttachmentOptimal, /* finalLayout */
            },
            // depth attachment
            {
                vk::AttachmentDescriptionFlags(),                /* flags */
                m_depth_format,                                  /* format */
                sample_count,                                    /* samples */
                vk::AttachmentLoadOp::eClear,                    /* loadOp */
                vk::AttachmentStoreOp::eStore,                   /* storeOp */
                vk::AttachmentLoadOp::eClear,                    /* stencilLoadOp */
                vk::AttachmentStoreOp::eStore,                   /* stencilStoreOp */
                vk::ImageLayout::eUndefined,                     /* initialLayout */
                vk::ImageLayout::eDepthStencilAttachmentOptimal, /* finalLayout */
            },
        };

        // Create reference to attachment information set

        vk::AttachmentReference swapchain_attachment_reference {0, vk::ImageLayout::eColorAttachmentOptimal};

        std::vector<vk::AttachmentReference> color_attachment_references {
            {1, vk::ImageLayout::eColorAttachmentOptimal},
            {2, vk::ImageLayout::eColorAttachmentOptimal},
            {3, vk::ImageLayout::eColorAttachmentOptimal},
        };

        vk::AttachmentReference depth_attachment_reference {4, vk::ImageLayout::eDepthStencilAttachmentOptimal};

        std::vector<vk::AttachmentReference> input_attachment_references {
            {1, vk::ImageLayout::eShaderReadOnlyOptimal},
            {2, vk::ImageLayout::eShaderReadOnlyOptimal},
            {3, vk::ImageLayout::eShaderReadOnlyOptimal},
            {4, vk::ImageLayout::eShaderReadOnlyOptimal},
        };

        // Create subpass

        std::vector<vk::SubpassDescription> subpass_descriptions {
            // obj2attachment pass
            {
                vk::SubpassDescriptionFlags(),    /* flags */
                vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                {},                               /* pInputAttachments */
                color_attachment_references,      /* pColorAttachments */
                {},                               /* pResolveAttachments */
                &depth_attachment_reference,      /* pDepthStencilAttachment */
                nullptr,                          /* pPreserveAttachments */
            },
            // quad rendering pass
            {
                vk::SubpassDescriptionFlags(),    /* flags */
                vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                input_attachment_references,      /* pInputAttachments */
                swapchain_attachment_reference,   /* pColorAttachments */
                {},                               /* pResolveAttachments */
                {},                               /* pDepthStencilAttachment */
                nullptr,                          /* pPreserveAttachments */
            },
            // skybox rendering pass
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

        // Create subpass dependency

        std::vector<vk::SubpassDependency> dependencies {
            // externel -> obj2attachment pass
            {
                VK_SUBPASS_EXTERNAL,                               /* srcSubpass */
                0,                                                 /* dstSubpass */
                vk::PipelineStageFlagBits::eBottomOfPipe,          /* srcStageMask */
                vk::PipelineStageFlagBits::eColorAttachmentOutput, /* dstStageMask */
                vk::AccessFlagBits::eMemoryRead,                   /* srcAccessMask */
                vk::AccessFlagBits::eColorAttachmentWrite,         /* dstAccessMask */
                vk::DependencyFlagBits::eByRegion,                 /* dependencyFlags */
            },
            // obj2attachment pass -> quad rendering pass
            {
                0,                                                 /* srcSubpass */
                1,                                                 /* dstSubpass */
                vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                vk::PipelineStageFlagBits::eFragmentShader,        /* dstStageMask */
                vk::AccessFlagBits::eColorAttachmentWrite,         /* srcAccessMask */
                vk::AccessFlagBits::eShaderRead,                   /* dstAccessMask */
                vk::DependencyFlagBits::eByRegion,                 /* dependencyFlags */
            },
            // quad rendering pass -> skybox rendering pass
            {
                1,                                                 /* srcSubpass */
                2,                                                 /* dstSubpass */
                vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                vk::PipelineStageFlagBits::eFragmentShader,        /* dstStageMask */
                vk::AccessFlagBits::eColorAttachmentWrite,         /* srcAccessMask */
                vk::AccessFlagBits::eShaderRead,                   /* dstAccessMask */
                vk::DependencyFlagBits::eByRegion,                 /* dependencyFlags */
            },
            // skybox rendering pass -> externel
            {
                2,                                                 /* srcSubpass */
                VK_SUBPASS_EXTERNAL,                               /* dstSubpass */
                vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                vk::PipelineStageFlagBits::eBottomOfPipe,          /* dstStageMask */
                vk::AccessFlagBits::eColorAttachmentWrite,         /* srcAccessMask */
                vk::AccessFlagBits::eMemoryRead,                   /* dstAccessMask */
                vk::DependencyFlagBits::eByRegion,                 /* dependencyFlags */
            },
        };

        // Create render pass
        vk::RenderPassCreateInfo render_pass_create_info(vk::RenderPassCreateFlags(), /* flags */
                                                         attachment_descriptions,     /* pAttachments */
                                                         subpass_descriptions,        /* pSubpasses */
                                                         dependencies);               /* pDependencies */

        render_pass = vk::raii::RenderPass(logical_device, render_pass_create_info);

        clear_values.resize(5);
        clear_values[0].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[1].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[2].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[3].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
        clear_values[4].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

        CreateMaterial();

        VkQueryPoolCreateInfo query_pool_create_info = {.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                                        .queryType          = VK_QUERY_TYPE_PIPELINE_STATISTICS,
                                                        .queryCount         = 2,
                                                        .pipelineStatistics = (1 << 11) - 1};

        query_pool = logical_device.createQueryPool(query_pool_create_info, nullptr);

        m_render_stat[0].vertex_attribute_metas = m_obj2attachment_material->shader->vertex_attribute_metas;
        m_render_stat[0].buffer_meta_map        = m_obj2attachment_material->shader->buffer_meta_map;
        m_render_stat[0].image_meta_map         = m_obj2attachment_material->shader->image_meta_map;

        m_render_stat[1].vertex_attribute_metas = m_quad_material->shader->vertex_attribute_metas;
        m_render_stat[1].buffer_meta_map        = m_quad_material->shader->buffer_meta_map;
        m_render_stat[1].image_meta_map         = m_quad_material->shader->image_meta_map;
    }

    void
    DeferredPassEditor::Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index)
    {
        if (m_query_enabled)
        {
            for (int i = 0; i < 2; i++)
            {
                command_buffer.resetQueryPool(*query_pool, 0, 2);
            }
        }

        DeferredPassBase::Start(command_buffer, extent, image_index);
    }

    void DeferredPassEditor::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_obj2attachment_material->BindPipeline(command_buffer);

        if (m_query_enabled)
            command_buffer.beginQuery(*query_pool, 0, {});

        RenderGBuffer(command_buffer);

        if (m_query_enabled)
            command_buffer.endQuery(*query_pool, 0);

        command_buffer.nextSubpass(vk::SubpassContents::eInline);

        m_quad_material->BindPipeline(command_buffer);

        if (m_query_enabled)
            command_buffer.beginQuery(*query_pool, 1, {});

        RenderOpaqueMeshes(command_buffer);

        if (m_query_enabled)
            command_buffer.endQuery(*query_pool, 1);

        command_buffer.nextSubpass(vk::SubpassContents::eInline);

        m_skybox_material->BindPipeline(command_buffer);

        RenderSkybox(command_buffer);
    }

    void DeferredPassEditor::AfterPresent()
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

    void swap(DeferredPassEditor& lhs, DeferredPassEditor& rhs)
    {
        using std::swap;

        swap(static_cast<DeferredPassBase&>(lhs), static_cast<DeferredPassBase&>(rhs));

        swap(lhs.m_query_enabled, rhs.m_query_enabled);
        swap(lhs.query_pool, rhs.query_pool);
        swap(lhs.m_render_stat, rhs.m_render_stat);
    }
} // namespace Meow
