#include "forward_pass_game.h"

#include "meow_runtime/pch.h"

#include "function/render/utils/vulkan_debug_utils.h"
#include "meow_runtime/function/global/runtime_context.h"

namespace Meow
{
    ForwardPassGame::ForwardPassGame(SurfaceData& surface_data)
        : ForwardPassBase(surface_data)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        CreateRenderPass();
        CreateMaterial();
    }

    void ForwardPassGame::CreateRenderPass()
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

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
                // swapchain image
                {
                    vk::AttachmentDescriptionFlags(), /* flags */
                    m_color_format,                   /* format */
                    vk::SampleCountFlagBits::e1,      /* samples */
                    vk::AttachmentLoadOp::eClear,     /* loadOp */
                    vk::AttachmentStoreOp::eStore,    /* storeOp */
                    vk::AttachmentLoadOp::eDontCare,  /* stencilLoadOp */
                    vk::AttachmentStoreOp::eDontCare, /* stencilStoreOp */
                    vk::ImageLayout::eUndefined,      /* initialLayout */
                    vk::ImageLayout::ePresentSrcKHR,  /* finalLayout */
                },
            };
        }
        else
        {
            attachment_descriptions = {
                {
                    vk::AttachmentDescriptionFlags(), /* flags */
                    m_color_format,                   /* format */
                    vk::SampleCountFlagBits::e1,      /* samples */
                    vk::AttachmentLoadOp::eClear,     /* loadOp */
                    vk::AttachmentStoreOp::eStore,    /* storeOp */
                    vk::AttachmentLoadOp::eDontCare,  /* stencilLoadOp */
                    vk::AttachmentStoreOp::eDontCare, /* stencilStoreOp */
                    vk::ImageLayout::eUndefined,      /* initialLayout */
                    vk::ImageLayout::ePresentSrcKHR,  /* finalLayout */
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
    ForwardPassGame::Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index)
    {
        ForwardPassBase::Start(command_buffer, extent, image_index);
    }

    void ForwardPassGame::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_opaque_material->BindPipeline(command_buffer);

        RenderOpaqueMeshes(command_buffer);

        m_skybox_material->BindPipeline(command_buffer);

        RenderSkybox(command_buffer);

        m_translucent_material->BindPipeline(command_buffer);
        RenderTranslucentMeshes(command_buffer);
    }
} // namespace Meow
