#include "deferred_pass_game.h"

#include "meow_runtime/pch.h"

#include "meow_runtime/function/global/runtime_context.h"

namespace Meow
{
    DeferredPassGame::DeferredPassGame(SurfaceData& surface_data)
        : DeferredPassBase(surface_data)
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

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
                vk::AttachmentDescriptionFlags(), /* flags */
                m_color_format,                   /* format */
                sample_count,                     /* samples */
                vk::AttachmentLoadOp::eClear,     /* loadOp */
                vk::AttachmentStoreOp::eStore,    /* storeOp */
                vk::AttachmentLoadOp::eDontCare,  /* stencilLoadOp */
                vk::AttachmentStoreOp::eDontCare, /* stencilStoreOp */
                vk::ImageLayout::eUndefined,      /* initialLayout */
                vk::ImageLayout::ePresentSrcKHR,  /* finalLayout */
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
    }

    void
    DeferredPassGame::Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index)
    {
        DeferredPassBase::Start(command_buffer, extent, image_index);
    }

    void DeferredPassGame::RecordGraphicsCommand(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index)
    {
        FUNCTION_TIMER();

        m_obj2attachment_material->BindPipeline(command_buffer);

        RenderGBuffer(command_buffer);

        command_buffer.nextSubpass(vk::SubpassContents::eInline);

        m_quad_material->BindPipeline(command_buffer);

        RenderOpaqueMeshes(command_buffer);

        command_buffer.nextSubpass(vk::SubpassContents::eInline);

        m_skybox_material->BindPipeline(command_buffer);

        RenderSkybox(command_buffer);
    }
} // namespace Meow
