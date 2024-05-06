#include "deferred_pass.h"
#include "function/global/runtime_global_context.h"

namespace Meow
{
    DeferredPass::DeferredPass(vk::raii::PhysicalDevice const& physical_device,
                               vk::raii::Device const&         device,
                               SurfaceData&                    surface_data,
                               vk::raii::CommandPool const&    command_pool,
                               vk::raii::Queue const&          queue,
                               DescriptorAllocatorGrowable&    m_descriptor_allocator)
    {
        // Create a set to store all information of attachments

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*surface_data.surface)).format;
        assert(color_format != vk::Format::eUndefined);

        std::vector<vk::AttachmentDescription> attachment_descriptions;
        // swap chain attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(), /* flags */
                                             color_format,                     /* format */
                                             sample_count,                     /* samples */
                                             vk::AttachmentLoadOp::eClear,     /* loadOp */
                                             vk::AttachmentStoreOp::eStore,    /* storeOp */
                                             vk::AttachmentLoadOp::eDontCare,  /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eDontCare, /* stencilStoreOp */
                                             vk::ImageLayout::eUndefined,      /* initialLayout */
                                             vk::ImageLayout::ePresentSrcKHR); /* finalLayout */
        // color attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),          /* flags */
                                             color_format,                              /* format */
                                             sample_count,                              /* samples */
                                             vk::AttachmentLoadOp::eClear,              /* loadOp */
                                             vk::AttachmentStoreOp::eStore,             /* storeOp */
                                             vk::AttachmentLoadOp::eDontCare,           /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eDontCare,          /* stencilStoreOp */
                                             vk::ImageLayout::eUndefined,               /* initialLayout */
                                             vk::ImageLayout::eColorAttachmentOptimal); /* finalLayout */
        // normal attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),          /* flags */
                                             vk::Format::eR8G8B8A8Unorm,                /* format */
                                             sample_count,                              /* samples */
                                             vk::AttachmentLoadOp::eClear,              /* loadOp */
                                             vk::AttachmentStoreOp::eStore,             /* storeOp */
                                             vk::AttachmentLoadOp::eDontCare,           /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eDontCare,          /* stencilStoreOp */
                                             vk::ImageLayout::eUndefined,               /* initialLayout */
                                             vk::ImageLayout::eColorAttachmentOptimal); /* finalLayout */
        // depth attachment
        attachment_descriptions.emplace_back(vk::AttachmentDescriptionFlags(),                 /* flags */
                                             depth_format,                                     /* format */
                                             sample_count,                                     /* samples */
                                             vk::AttachmentLoadOp::eClear,                     /* loadOp */
                                             vk::AttachmentStoreOp::eStore,                    /* storeOp */
                                             vk::AttachmentLoadOp::eClear,                     /* stencilLoadOp */
                                             vk::AttachmentStoreOp::eStore,                    /* stencilStoreOp */
                                             vk::ImageLayout::eUndefined,                      /* initialLayout */
                                             vk::ImageLayout::eDepthStencilAttachmentOptimal); /* finalLayout */

        // Create reference to attachment information set

        vk::AttachmentReference swapchain_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);

        std::vector<vk::AttachmentReference> color_attachment_references;
        color_attachment_references.emplace_back(1, vk::ImageLayout::eColorAttachmentOptimal);
        color_attachment_references.emplace_back(2, vk::ImageLayout::eColorAttachmentOptimal);

        vk::AttachmentReference depth_attachment_reference(3, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        std::vector<vk::AttachmentReference> input_attachment_references;
        input_attachment_references.emplace_back(1, vk::ImageLayout::eShaderReadOnlyOptimal);
        input_attachment_references.emplace_back(2, vk::ImageLayout::eShaderReadOnlyOptimal);
        input_attachment_references.emplace_back(3, vk::ImageLayout::eShaderReadOnlyOptimal);

        // Create subpass

        std::vector<vk::SubpassDescription> subpass_descriptions;
        // obj2attachment pass
        subpass_descriptions.push_back(vk::SubpassDescription(vk::SubpassDescriptionFlags(),    /* flags */
                                                              vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                                                              {},                               /* pInputAttachments */
                                                              color_attachment_references,      /* pColorAttachments */
                                                              {},                          /* pResolveAttachments */
                                                              &depth_attachment_reference, /* pDepthStencilAttachment */
                                                              nullptr));                   /* pPreserveAttachments */
        // quad renderering pass
        subpass_descriptions.push_back(vk::SubpassDescription(vk::SubpassDescriptionFlags(),    /* flags */
                                                              vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                                                              input_attachment_references,      /* pInputAttachments */
                                                              swapchain_attachment_reference,   /* pColorAttachments */
                                                              {},        /* pResolveAttachments */
                                                              {},        /* pDepthStencilAttachment */
                                                              nullptr)); /* pPreserveAttachments */

        // Create subpass dependency

        std::vector<vk::SubpassDependency> dependencies;
        // externel -> obj2attachment pass
        dependencies.emplace_back(VK_SUBPASS_EXTERNAL,                               /* srcSubpass */
                                  0,                                                 /* dstSubpass */
                                  vk::PipelineStageFlagBits::eBottomOfPipe,          /* srcStageMask */
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput, /* dstStageMask */
                                  vk::AccessFlagBits::eMemoryRead,                   /* srcAccessMask */
                                  vk::AccessFlagBits::eColorAttachmentWrite,         /* dstAccessMask */
                                  vk::DependencyFlagBits::eByRegion);                /* dependencyFlags */
        // obj2attachment pass -> quad renderering pass
        dependencies.emplace_back(0,                                                 /* srcSubpass */
                                  1,                                                 /* dstSubpass */
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                                  vk::PipelineStageFlagBits::eFragmentShader,        /* dstStageMask */
                                  vk::AccessFlagBits::eColorAttachmentWrite,         /* srcAccessMask */
                                  vk::AccessFlagBits::eShaderRead,                   /* dstAccessMask */
                                  vk::DependencyFlagBits::eByRegion);                /* dependencyFlags */
        // quad renderering pass -> externel
        dependencies.emplace_back(1,                                                 /* srcSubpass */
                                  VK_SUBPASS_EXTERNAL,                               /* dstSubpass */
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                                  vk::PipelineStageFlagBits::eFragmentShader,        /* dstStageMask */
                                  vk::AccessFlagBits::eColorAttachmentWrite,         /* srcAccessMask */
                                  vk::AccessFlagBits::eShaderRead,                   /* dstAccessMask */
                                  vk::DependencyFlagBits::eByRegion);                /* dependencyFlags */

        // Create render pass
        vk::RenderPassCreateInfo render_pass_create_info(vk::RenderPassCreateFlags(), /* flags */
                                                         attachment_descriptions,     /* pAttachments */
                                                         subpass_descriptions,        /* pSubpasses */
                                                         dependencies);               /* pDependencies */

        render_pass = vk::raii::RenderPass(device, render_pass_create_info);

        // ImGui render pass

        // swap chain attachment
        vk::AttachmentDescription imgui_attachment_description(vk::AttachmentDescriptionFlags(), /* flags */
                                                               color_format,                     /* format */
                                                               sample_count,                     /* samples */
                                                               vk::AttachmentLoadOp::eClear,     /* loadOp */
                                                               vk::AttachmentStoreOp::eStore,    /* storeOp */
                                                               vk::AttachmentLoadOp::eDontCare,  /* stencilLoadOp */
                                                               vk::AttachmentStoreOp::eDontCare, /* stencilStoreOp */
                                                               vk::ImageLayout::eUndefined,      /* initialLayout */
                                                               vk::ImageLayout::ePresentSrcKHR); /* finalLayout */

        vk::SubpassDescription imgui_subpass_description(
            vk::SubpassDescription(vk::SubpassDescriptionFlags(),    /* flags */
                                   vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                                   {},                               /* pInputAttachments */
                                   swapchain_attachment_reference,   /* pColorAttachments */
                                   {},                               /* pResolveAttachments */
                                   {},                               /* pDepthStencilAttachment */
                                   nullptr));                        /* pPreserveAttachments */

        vk::SubpassDependency imgui_dependency(VK_SUBPASS_EXTERNAL,                               /* srcSubpass */
                                               0,                                                 /* dstSubpass */
                                               vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                                               vk::PipelineStageFlagBits::eColorAttachmentOutput, /* dstStageMask */
                                               {},                                                /* srcAccessMask */
                                               vk::AccessFlagBits::eColorAttachmentWrite,         /* dstAccessMask */
                                               {});                                               /* dependencyFlags */

        vk::RenderPassCreateInfo imgui_render_pass_create_info(vk::RenderPassCreateFlags(),  /* flags */
                                                               imgui_attachment_description, /* pAttachments */
                                                               imgui_subpass_description,    /* pSubpasses */
                                                               imgui_dependency);            /* pDependencies */

        imgui_pass = vk::raii::RenderPass(device, imgui_render_pass_create_info);

        // Create Material

        std::shared_ptr<Shader> obj_shader_ptr = std::make_shared<Shader>(physical_device,
                                                                          device,
                                                                          m_descriptor_allocator,
                                                                          "builtin/shaders/obj.vert.spv",
                                                                          "builtin/shaders/obj.frag.spv");

        obj2attachment_mat                        = Material(physical_device, device, obj_shader_ptr);
        obj2attachment_mat.color_attachment_count = 2;
        obj2attachment_mat.CreatePipeline(device, render_pass, vk::FrontFace::eClockwise, true);

        std::shared_ptr<Shader> quad_shader_ptr = std::make_shared<Shader>(physical_device,
                                                                           device,
                                                                           m_descriptor_allocator,
                                                                           "builtin/shaders/quad.vert.spv",
                                                                           "builtin/shaders/quad.frag.spv");

        quad_mat         = Material(physical_device, device, quad_shader_ptr);
        quad_mat.subpass = 1;
        quad_mat.CreatePipeline(device, render_pass, vk::FrontFace::eClockwise, false);

        // Create quad model
        std::vector<float>    vertices = {-1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f, 1.0f, 0.0f,
                                          1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f};
        std::vector<uint16_t> indices  = {0, 1, 2, 0, 2, 3};

        quad_model = std::move(Model(physical_device,
                                     device,
                                     command_pool,
                                     queue,
                                     vertices,
                                     indices,
                                     quad_mat.shader_ptr->per_vertex_attributes));

        clear_values[0].color        = vk::ClearColorValue(0.2f, 0.2f, 0.2f, 0.2f);
        clear_values[1].color        = vk::ClearColorValue(0.2f, 0.2f, 0.2f, 0.2f);
        clear_values[2].color        = vk::ClearColorValue(0.2f, 0.2f, 0.2f, 0.2f);
        clear_values[3].color        = vk::ClearColorValue(0.2f, 0.2f, 0.2f, 0.2f);
        clear_values[4].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
    }

    void DeferredPass::RefreshFrameBuffers(vk::raii::PhysicalDevice const&         physical_device,
                                           vk::raii::Device const&                 device,
                                           vk::raii::CommandBuffer const&          command_buffer,
                                           SurfaceData&                            surface_data,
                                           std::vector<vk::raii::ImageView> const& swapchain_image_views,
                                           vk::Extent2D const&                     extent)
    {
        // clear

        framebuffers.clear();

        m_color_attachment  = nullptr;
        m_normal_attachment = nullptr;
        m_depth_attachment  = nullptr;

        // Create attachment

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*surface_data.surface)).format;

        m_color_attachment = ImageData::CreateAttachment(physical_device,
                                                         device,
                                                         command_buffer,
                                                         color_format,
                                                         extent,
                                                         vk::ImageUsageFlagBits::eColorAttachment |
                                                             vk::ImageUsageFlagBits::eInputAttachment,
                                                         vk::ImageAspectFlagBits::eColor,
                                                         {},
                                                         false);

        m_normal_attachment = ImageData::CreateAttachment(physical_device,
                                                          device,
                                                          command_buffer,
                                                          vk::Format::eR8G8B8A8Unorm,
                                                          extent,
                                                          vk::ImageUsageFlagBits::eColorAttachment |
                                                              vk::ImageUsageFlagBits::eInputAttachment,
                                                          vk::ImageAspectFlagBits::eColor,
                                                          {},
                                                          false);

        m_depth_attachment = ImageData::CreateAttachment(physical_device,
                                                         device,
                                                         command_buffer,
                                                         depth_format,
                                                         extent,
                                                         vk::ImageUsageFlagBits::eDepthStencilAttachment |
                                                             vk::ImageUsageFlagBits::eInputAttachment,
                                                         vk::ImageAspectFlagBits::eDepth,
                                                         {},
                                                         false);

        // Provide attachment information to frame buffer

        vk::ImageView attachments[4];
        attachments[1] = *m_color_attachment->image_view;
        attachments[2] = *m_normal_attachment->image_view;
        attachments[3] = *m_depth_attachment->image_view;

        vk::FramebufferCreateInfo framebuffer_create_info(vk::FramebufferCreateFlags(), /* flags */
                                                          *render_pass,                 /* renderPass */
                                                          4,                            /* attachmentCount */
                                                          attachments,                  /* pAttachments */
                                                          extent.width,                 /* width */
                                                          extent.height,                /* height */
                                                          1);                           /* layers */

        framebuffers.reserve(swapchain_image_views.size());
        for (auto const& imageView : swapchain_image_views)
        {
            attachments[0] = *imageView;
            framebuffers.push_back(vk::raii::Framebuffer(device, framebuffer_create_info));
        }

        // Update descriptor set

        quad_mat.SetImage(device, "inputColor", *m_color_attachment);
        quad_mat.SetImage(device, "inputNormal", *m_normal_attachment);
        quad_mat.SetImage(device, "inputDepth", *m_depth_attachment);
    }
} // namespace Meow
