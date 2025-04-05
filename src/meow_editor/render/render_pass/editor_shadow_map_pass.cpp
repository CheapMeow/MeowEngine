#include "editor_shadow_map_pass.h"

#include "meow_runtime/pch.h"

#include "function/render/geometry/geometry_factory.h"
#include "function/render/material/material_factory.h"
#include "function/render/utils/vulkan_debug_utils.h"
#include "global/editor_context.h"
#include "meow_runtime/function/global/runtime_context.h"

namespace Meow
{
    EditorShadowMapPass::EditorShadowMapPass(SurfaceData& surface_data)
        : ShadowMapPass(surface_data)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        CreateRenderPass();
        CreateMaterial();

#ifdef MEOW_EDITOR
        VkQueryPoolCreateInfo query_pool_create_info = {.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                                        .queryType          = VK_QUERY_TYPE_PIPELINE_STATISTICS,
                                                        .queryCount         = 1,
                                                        .pipelineStatistics = (1 << 11) - 1};

        query_pool = logical_device.createQueryPool(query_pool_create_info, nullptr);

        m_render_stat[0].vertex_attribute_metas = m_shadow_map_material->shader->vertex_attribute_metas;
        m_render_stat[0].buffer_meta_map        = m_shadow_map_material->shader->buffer_meta_map;
        m_render_stat[0].image_meta_map         = m_shadow_map_material->shader->image_meta_map;
#endif
    }

    void EditorShadowMapPass::CreateRenderPass()
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

#ifdef MEOW_EDITOR
        m_pass_names[0] = "Shadow Map Subpass";
#endif

        // Create a set to store all information of attachments

        std::vector<vk::AttachmentDescription> attachment_descriptions;

        attachment_descriptions = {
            // depth attachment
            {
                vk::AttachmentDescriptionFlags(),        /* flags */
                m_depth_format,                          /* format */
                vk::SampleCountFlagBits::e1,             /* samples */
                vk::AttachmentLoadOp::eClear,            /* loadOp */
                vk::AttachmentStoreOp::eStore,           /* storeOp */
                vk::AttachmentLoadOp::eClear,            /* stencilLoadOp */
                vk::AttachmentStoreOp::eStore,           /* stencilStoreOp */
                vk::ImageLayout::eUndefined,             /* initialLayout */
                vk::ImageLayout::eShaderReadOnlyOptimal, /* finalLayout */
            },
#ifdef MEOW_EDITOR
            // depth to color attachment
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
#endif
        };

        vk::AttachmentReference depth_attachment_reference(0, vk::ImageLayout::eDepthStencilAttachmentOptimal);
#ifdef MEOW_EDITOR
        std::vector<vk::AttachmentReference> color_attachment_references {
            {0, vk::ImageLayout::eShaderReadOnlyOptimal},
            {1, vk::ImageLayout::eColorAttachmentOptimal},
        };
#endif

        std::vector<vk::SubpassDescription> subpass_descriptions {
            // shadow map pass
            {
                vk::SubpassDescriptionFlags(),    /* flags */
                vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                0,                                /* inputAttachmentCount */
                nullptr,                          /* pInputAttachments */
                0,                                /* colorAttachmentCount */
                nullptr,                          /* pColorAttachments */
                nullptr,                          /* pResolveAttachments */
                &depth_attachment_reference,      /* pDepthStencilAttachment */
                0,                                /* preserveAttachmentCount */
                nullptr,                          /* pPreserveAttachments */
            },
#ifdef MEOW_EDITOR
            // depth to color pass
            {
                vk::SubpassDescriptionFlags(),    /* flags */
                vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                0,                                /* inputAttachmentCount */
                nullptr,                          /* pInputAttachments */
                2,                                /* colorAttachmentCount */
                color_attachment_references,      /* pColorAttachments */
                nullptr,                          /* pResolveAttachments */
                nullptr,                          /* pDepthStencilAttachment */
                0,                                /* preserveAttachmentCount */
                nullptr,                          /* pPreserveAttachments */
            },
#endif
        };

        std::vector<vk::SubpassDependency> dependencies {
            // externel -> shadow map pass
            {
                VK_SUBPASS_EXTERNAL,                              /* srcSubpass */
                0,                                                /* dstSubpass */
                vk::PipelineStageFlagBits::eFragmentShader,       /* srcStageMask */
                vk::PipelineStageFlagBits::eEarlyFragmentTests,   /* dstStageMask */
                vk::AccessFlagBits::eShaderRead,                  /* srcAccessMask */
                vk::AccessFlagBits::eDepthStencilAttachmentWrite, /* dstAccessMask */
                vk::DependencyFlagBits::eByRegion,                /* dependencyFlags */
            },
#ifndef MEOW_EDITOR
            // shadow map pass -> externel
            {
                0,                                                /* srcSubpass */
                VK_SUBPASS_EXTERNAL,                              /* dstSubpass */
                vk::PipelineStageFlagBits::eLateFragmentTests,    /* srcStageMask */
                vk::PipelineStageFlagBits::eFragmentShader,       /* dstStageMask */
                vk::AccessFlagBits::eDepthStencilAttachmentWrite, /* srcAccessMask */
                vk::AccessFlagBits::eShaderRead,                  /* dstAccessMask */
                vk::DependencyFlagBits::eByRegion,                /* dependencyFlags */
            },
#else
            // shadow map pass -> depth to color pass
            {
                0,                                                /* srcSubpass */
                1,                                                /* dstSubpass */
                vk::PipelineStageFlagBits::eLateFragmentTests,    /* srcStageMask */
                vk::PipelineStageFlagBits::eFragmentShader,       /* dstStageMask */
                vk::AccessFlagBits::eDepthStencilAttachmentWrite, /* srcAccessMask */
                vk::AccessFlagBits::eShaderRead,                  /* dstAccessMask */
                vk::DependencyFlagBits::eByRegion,                /* dependencyFlags */
            },
            // depth to color pass -> externel
            {
                1,                                                 /* srcSubpass */
                VK_SUBPASS_EXTERNAL,                               /* dstSubpass */
                vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                vk::PipelineStageFlagBits::eFragmentShader,        /* dstStageMask */
                vk::AccessFlagBits::eColorAttachmentWrite,         /* srcAccessMask */
                vk::AccessFlagBits::eShaderRead,                   /* dstAccessMask */
                vk::DependencyFlagBits::eByRegion,                 /* dependencyFlags */
            },
#endif
        };

        // Create render pass
        vk::RenderPassCreateInfo render_pass_create_info(vk::RenderPassCreateFlags(), /* flags */
                                                         attachment_descriptions,     /* pAttachments */
                                                         subpass_descriptions,        /* pSubpasses */
                                                         dependencies);               /* pDependencies */

        render_pass = vk::raii::RenderPass(logical_device, render_pass_create_info);

#ifndef MEOW_EDITOR
        clear_values.resize(1);
        clear_values[0].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
#else
        clear_values.resize(2);
        clear_values[0].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
        clear_values[1].color        = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
#endif

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::DebugUtilsObjectNameInfoEXT name_info = {vk::ObjectType::eRenderPass,
                                                     NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkRenderPass, *render_pass),
                                                     "Shadow Map RenderPass"};
        logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif
    }

#ifdef MEOW_EDITOR
    void EditorShadowMapPass::CreateMaterial()
    {
        ShadowMapPass::CreateMaterial();

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

        m_depth_to_color_material->BindImageToDescriptorSet("inputDepth", *m_shadow_map);

        // Create quad model
        std::vector<float>    vertices = {-1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 1.0f,  1.0f,  0.0f, 1.0f, 0.0f,
                                          1.0f,  -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f};
        std::vector<uint32_t> indices  = {0, 1, 2, 0, 2, 3};

        m_quad_model = std::move(
            Model(std::move(vertices), std::move(indices), m_depth_to_color_material->shader->per_vertex_attributes));
    }
#endif

    void EditorShadowMapPass::Start(const vk::raii::CommandBuffer& command_buffer,
                                    vk::Extent2D                   extent,
                                    uint32_t                       current_image_index)
    {
#ifdef MEOW_EDITOR
        if (m_query_enabled)
            command_buffer.resetQueryPool(*query_pool, 0, 1);
#endif

        ShadowMapPass::Start(command_buffer, extent, current_image_index);
    }

    void EditorShadowMapPass::Draw(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_shadow_map_material->BindPipeline(command_buffer);

#ifdef MEOW_EDITOR
        if (m_query_enabled)
            command_buffer.beginQuery(*query_pool, 0, {});
#endif

        RenderShadowMap(command_buffer);

#ifdef MEOW_EDITOR
        if (m_query_enabled)
            command_buffer.endQuery(*query_pool, 0);
#endif

#ifdef MEOW_EDITOR
        command_buffer.nextSubpass(vk::SubpassContents::eInline);

        m_depth_to_color_material->BindPipeline(command_buffer);

        RenderDepthToColor(command_buffer);
#endif
    }

#ifdef MEOW_EDITOR
    void EditorShadowMapPass::RenderDepthToColor(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        m_depth_to_color_material->BindDescriptorSetToPipeline(command_buffer, 0, 1);
        m_quad_model.meshes[0]->BindDrawCmd(command_buffer);
    }
#endif

    void EditorShadowMapPass::AfterPresent()
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

    void swap(EditorShadowMapPass& lhs, EditorShadowMapPass& rhs)
    {
        using std::swap;

#ifdef MEOW_EDITOR
        swap(lhs.m_query_enabled, rhs.m_query_enabled);
        swap(lhs.query_pool, rhs.query_pool);
        swap(lhs.m_render_stat, rhs.m_render_stat);

        swap(lhs.m_depth_to_color_material, rhs.m_depth_to_color_material);
        swap(lhs.m_quad_model, rhs.m_quad_model);
#endif
    }
} // namespace Meow
