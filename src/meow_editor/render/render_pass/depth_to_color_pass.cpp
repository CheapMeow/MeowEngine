#include "depth_to_color_pass.h"

#include "pch.h"

#include "function/global/runtime_context.h"
#include "function/render/geometry/geometry_factory.h"
#include "function/render/material/material_factory.h"
#include "function/render/utils/vulkan_debug_utils.h"
#include "global/editor_context.h"

namespace Meow
{
    DepthToColorPass::DepthToColorPass(SurfaceData& surface_data)
        : RenderPass(surface_data)
    {
        CreateRenderPass();
        CreateMaterial();
    }

    void DepthToColorPass::CreateRenderPass()
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        // Create a set to store all information of attachments

        std::vector<vk::AttachmentDescription> attachment_descriptions;

        attachment_descriptions = {
            // color attachment
            {
                vk::AttachmentDescriptionFlags(),         /* flags */
                m_color_format,                           /* format */
                vk::SampleCountFlagBits::e1,              /* samples */
                vk::AttachmentLoadOp::eClear,             /* loadOp */
                vk::AttachmentStoreOp::eStore,            /* storeOp */
                vk::AttachmentLoadOp::eDontCare,          /* stencilLoadOp */
                vk::AttachmentStoreOp::eDontCare,         /* stencilStoreOp */
                vk::ImageLayout::eUndefined,              /* initialLayout */
                vk::ImageLayout::eColorAttachmentOptimal, /* finalLayout */
            },
        };
        vk::AttachmentReference color_attachment_reference = {0, vk::ImageLayout::eShaderReadOnlyOptimal};

        std::vector<vk::SubpassDescription> subpass_descriptions {
            // depth to color pass
            {
                vk::SubpassDescriptionFlags(),    /* flags */
                vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                0,                                /* inputAttachmentCount */
                nullptr,                          /* pInputAttachments */
                1,                                /* colorAttachmentCount */
                &color_attachment_reference,      /* pColorAttachments */
                nullptr,                          /* pResolveAttachments */
                nullptr,                          /* pDepthStencilAttachment */
                0,                                /* preserveAttachmentCount */
                nullptr,                          /* pPreserveAttachments */
            },
        };

        std::vector<vk::SubpassDependency> dependencies {
            // externel -> depth to color pass
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
            // depth to color pass -> externel
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

        clear_values.resize(1);
        clear_values[0].color = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::DebugUtilsObjectNameInfoEXT name_info = {vk::ObjectType::eRenderPass,
                                                     NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkRenderPass, *render_pass),
                                                     "Depth To Color RenderPass"};
        logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif
    }

    void DepthToColorPass::CreateMaterial()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();

        MaterialFactory material_factory;

        auto depth_to_color_shader = std::make_shared<Shader>(physical_device,
                                                              logical_device,
                                                              "builtin/shaders/depth_to_color.vert.spv",
                                                              "builtin/shaders/depth_to_color.frag.spv");
        m_depth_to_color_material  = std::make_shared<Material>(depth_to_color_shader);
        g_runtime_context.resource_system->Register(m_depth_to_color_material);
        material_factory.Init(depth_to_color_shader.get(), vk::FrontFace::eClockwise);
        material_factory.SetOpaque(false, 1);
        material_factory.CreatePipeline(
            logical_device, render_pass, depth_to_color_shader.get(), m_depth_to_color_material.get(), 1);
        m_depth_to_color_material->SetDebugName("Depth to Color Material");

        m_depth_to_color_render_target = ImageData::CreateRenderTarget(m_color_format,
                                                                       {2048, 2048},
                                                                       vk::ImageUsageFlagBits::eColorAttachment |
                                                                           vk::ImageUsageFlagBits::eInputAttachment,
                                                                       vk::ImageAspectFlagBits::eColor,
                                                                       {},
                                                                       false);

        // Create quad model
        std::vector<float>    vertices = {-1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f, 1.0f, 0.0f,
                                          1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f};
        std::vector<uint32_t> indices  = {0, 1, 2, 0, 2, 3};

        m_quad_model = std::move(
            Model(std::move(vertices), std::move(indices), m_depth_to_color_material->shader->per_vertex_attributes));
    }

    void DepthToColorPass::Start(const vk::raii::CommandBuffer& command_buffer,
                                 vk::Extent2D                   extent,
                                 uint32_t                       current_image_index)
    {
        RenderPass::Start(command_buffer, extent, current_image_index);
    }

    void DepthToColorPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_depth_to_color_material->BindPipeline(command_buffer);
        m_depth_to_color_material->BindDescriptorSetToPipeline(command_buffer, 0, 1);
        m_quad_model.meshes[0]->BindDrawCmd(command_buffer);
    }

    void DepthToColorPass::BindShadowMap(std::shared_ptr<ImageData> shadow_map)
    {
        m_depth_to_color_material->BindImageToDescriptorSet("inputDepth", *shadow_map);
    }

    void swap(DepthToColorPass& lhs, DepthToColorPass& rhs)
    {
        using std::swap;

        swap(lhs.m_depth_to_color_material, rhs.m_depth_to_color_material);
        swap(lhs.m_depth_to_color_render_target, rhs.m_depth_to_color_render_target);
        swap(lhs.m_quad_model, rhs.m_quad_model);
    }
} // namespace Meow
