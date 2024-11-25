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

        std::vector<vk::AttachmentDescription> attachment_descriptions {
            // swap chain attachment
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
            // color attachment
            {
                vk::AttachmentDescriptionFlags(),         /* flags */
                color_format,                             /* format */
                m_sample_count,                           /* samples */
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
                m_sample_count,                           /* samples */
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
                m_sample_count,                           /* samples */
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
                m_sample_count,                                  /* samples */
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
            // quad rendering pass -> externel
            {
                1,                                                 /* srcSubpass */
                VK_SUBPASS_EXTERNAL,                               /* dstSubpass */
                vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                vk::PipelineStageFlagBits::eFragmentShader,        /* dstStageMask */
                vk::AccessFlagBits::eColorAttachmentWrite,         /* srcAccessMask */
                vk::AccessFlagBits::eShaderRead,                   /* dstAccessMask */
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

        CreateMaterial(physical_device, logical_device, command_pool, queue, descriptor_allocator);

        // Debug

        VkQueryPoolCreateInfo query_pool_create_info = {.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                                        .queryType          = VK_QUERY_TYPE_PIPELINE_STATISTICS,
                                                        .queryCount         = 2,
                                                        .pipelineStatistics = (1 << 11) - 1};

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
